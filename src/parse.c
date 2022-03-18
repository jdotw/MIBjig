#define _GNU_SOURCE     // for asprintf
#include <stdio.h>
#include <pcre.h>
#include <unistd.h>
#include <sys/mman.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <string.h>
#include <net-snmp/net-snmp-config.h>
#ifdef HAVE_SIGNAL
#include <signal.h>
#endif /* HAVE_SIGNAL */
#include <errno.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

#include "hashtable.h"
#include "value.h"

#define OVECCOUNT 30

/*
 * Parse the SNMP Walk output, register the SNMP OIDs and store 
 * the values in the hashtable
 */

char* string_at_vector(char *source, int *vectors, int index)
{
  char *str;
  if (asprintf(&str, "%.*s", (int) (vectors[index+1] - vectors[index]), source + vectors[index]) < 0) {
    return NULL;
  }
  int i;
  for (i=strlen(str)-1; i >= 0; i--)
  {
    if (str[i] == ' ') str[i] = '\0';
    else if (str[i] == '\"') str[i] = '\0';
    else break;
  }
  return str;
}

oid* oid_from_string(char *str, size_t *oid_len)
{
  /* Returns a malloc'd oid[] based on the numeric format OID string str */

  /* Create a working copy of the str */
  str = strdup(str);

  /* Determine oid length by counting the . in the string */
  char *substring_vectors[strlen(str)];   // It doesn't need to be this big, but this value is safe
  *oid_len = 0;
  int i;
  size_t str_len = strlen(str);           // Must be saved here because it will change with substring termination
  for (i=0; i < str_len; i++)
  {
    if (str[i] == '.') 
    {
      substring_vectors[*oid_len] = str+i+1;  /* Record the start of the OID substring */
      *oid_len = (*oid_len) + 1;
      str[i] = '\0';  // Null terminate the substring
    }
  }

  /* Malloc new oid array */
  oid *oid_array = (oid *) calloc(*oid_len, sizeof(oid));

  /* Parse each OID substring */
  for (i=0; i < *oid_len; i++)
  {
    oid_array[i] = atoi(substring_vectors[i]);
  }

  /* Clean up */
  free(str);

  return oid_array;
}

void* hex_data_from_string(char *hex_string, size_t *hex_len)
{
  /* Converts a "20 2B 00 05 1E 36 75 10" style string into 
   * an array of bytes that represent the hex string
   */

  int i;
  char *hex_array = malloc(strlen(hex_string));   // Larger than needed, but safe
  memset(hex_array, 0, strlen(hex_string));
  *hex_len = 0;
  for (i=0; i < strlen(hex_string); i=i+3)
  {
    char *hex_substring;
    if (asprintf(&hex_substring, "0x%.*s", 2, hex_string+i) < 0) {
      free(hex_array);
      return NULL;
    }
    hex_array[*hex_len] = (u_char) strtol(hex_substring, NULL, 16);
    *hex_len = (*hex_len) + 1;
    free (hex_substring);
  }

  return hex_array;
}

void* timeticks_from_string(char *str, size_t *val_len)
{
  /* Create working copy */
  str = strdup(str);

  /* Scan string */
  int i;
  size_t str_len = strlen(str);
  char *ticks_start = str;
  for(i=0; i < str_len; i++)
  {
    if (str[i] == '(') ticks_start = str+i+1;
    else if (str[i] == ')') str[i] = '\0';
  }
  uint32_t ticks = (uint32_t) atoi(ticks_start);
  free (str);

  /* Set val and length */
  *val_len = sizeof(uint32_t);
  void *val = malloc(*val_len);
  memcpy(val, &ticks, *val_len);

  return val;
}

m_value* value_from_string(char *type_str, char *val_str)
{
  /* Returns a malloc'd m_value with the val and val_len values
   * set and ready to be used by the SNMP handler. The storage of
   * val->val and the length will be dependent on the type to be 
   * parsed from type_str. E.g ASN_GAUGE values will need to be 
   * converted from a string value to an integer
   */

  m_value *val = m_value_create();

  /* Dertermine ASN value type from type string */
  if (strcmp(type_str, "INTEGER")==0) val->type = ASN_INTEGER;
  else if (strcmp(type_str, "STRING")==0) val->type = ASN_OCTET_STR;
  else if (strcmp(type_str, "Hex-STRING")==0) val->type = ASN_OCTET_STR;
  else if (strcmp(type_str, "Timeticks")==0) val->type = ASN_TIMETICKS;
  else if (strcmp(type_str, "OID")==0) val->type = ASN_OBJECT_ID;
  else if (strcmp(type_str, "Gauge32")==0) val->type = ASN_GAUGE;
  else if (strcmp(type_str, "OID")==0) val->type = ASN_OBJECT_ID;
  else if (strcmp(type_str, "Counter32")==0) val->type = ASN_COUNTER;
  else if (strcmp(type_str, "Counter64")==0) val->type = ASN_COUNTER64;
  else if (strcmp(type_str, "IpAddress")==0) val->type = ASN_IPADDRESS;
  else if (strcmp(type_str, "Timeticks")==0) val->type = ASN_TIMETICKS;
  else if (strlen(type_str)==0) val->type = ASN_OCTET_STR;
  else 
  {
    snmp_log(LOG_ERR, "*** Unknown value type %s encountered, using string representation\n", type_str);
    val->type = ASN_OCTET_STR;
  }

  /* Set value according to the ASN type */
  uint32_t int32 = 0;
  in_addr_t in_addr;
  switch (val->type)
  {
    case ASN_INTEGER:
    case ASN_GAUGE:
    case ASN_COUNTER:
      int32 = atoi(val_str);
      val->val = malloc(sizeof(uint32_t));
      memcpy(val->val, &int32, sizeof(uint32_t));
      val->val_len = sizeof(uint32_t);
      break;
    case ASN_COUNTER64:
      /* net-snmp has an internal representation of counter64 */
      val->val = malloc(sizeof(struct counter64));
      read64(val->val, val_str);
      val->val_len = sizeof(struct counter64);
      break;
    case ASN_OCTET_STR:
      if (strcmp(type_str, "Hex-STRING")==0)
      {
        val->val = hex_data_from_string(val_str, &val->val_len);
      }
      else
      {
        val->val = strdup(val_str);
        val->val_len = strlen(val_str);
      }
      break;
    case ASN_IPADDRESS:
      in_addr = inet_addr(val_str);
      val->val = malloc(sizeof(in_addr_t));
      memcpy(val->val, &in_addr, sizeof(in_addr_t));
      val->val_len = sizeof(in_addr_t);
      break;
    case ASN_TIMETICKS:
      val->val = timeticks_from_string(val_str, &val->val_len);
      break;
    case ASN_OBJECT_ID:
      val->val = oid_from_string(val_str, &val->val_len);
      break;
    default:
      snmp_log(LOG_ERR, "*** Failed to create value for %s/%s (val->type is unknown)\n", type_str, val_str);
      return NULL;
  }

  return val;
}

void m_parse (char *filename)
{
  /* Open File */
  int fd = open(filename, O_RDONLY);
  if (fd == -1) 
  { snmp_log(LOG_ERR, "m_parse failed to open %s: %s\n", filename, strerror(errno)); return; }
  size_t filesize = lseek(fd, 0, SEEK_END);
  void *source = mmap(0, filesize, PROT_READ, MAP_SHARED, fd, 0);
  if (source == MAP_FAILED)
  { snmp_log(LOG_ERR, "m_parse failed to mmap %s: %s\n", filename, strerror(errno)); return; }

  /* Compile the line-matching RegExp.
   * Matches lines like:
   *
   * .1.3.6.1.3.94.1.6.1.5.16.0.0.5.30.54.117.16.0.0.0.0.0.0.0.0 = INTEGER: 2
   *
   * Captures OID String, Type and Value
   */
  const char *error;
  int erroffset;
  pcre *re = pcre_compile("^((?:\\.[0-9]+)+) =(?: ([\\w-]+)\\:)? \"?(.*)$", PCRE_MULTILINE, &error, &erroffset, NULL);

  if (!re) { snmp_log(LOG_ERR, "m_parse failed to compile line-matching RE\n"); return; }

  /* Parse the mmap'd input file */
  int ovector[OVECCOUNT];
  ovector[0] = 0;
  ovector[1] = 0;
  while (1)
  {
    int options = 0; 
    int start_offset = ovector[1];

    int rc = pcre_exec(
      re,                   /* the compiled pattern */
      NULL,                 /* no extra data - we didn't study the pattern */
      source,               /* the subject string */
      filesize,             /* the length of the subject */
      start_offset,         /* starting offset in the subject */
      options,              /* options */
      ovector,              /* output vector for substring information */
      OVECCOUNT);           /* number of elements in the output vector */

    if (rc == PCRE_ERROR_NOMATCH)
    {
      if (options == 0) break;
      ovector[1] = start_offset + 1;
      continue;    /* Go round the loop again */
    }

    /* Other matching errors are not recoverable. */
    if (rc > 0)
    {
      /* Matched a walk line
       * 0=FullString 1(2)=OID 2(4)=Type 3(6)=Value
       */
      char *oid_str = string_at_vector(source, ovector, 2);
      char *type_str = string_at_vector(source, ovector, 4);
      char *value_str = string_at_vector(source, ovector, 6);

      DEBUGMSGTL(("mibjig:parse", "OID='%s' TYPE='%s' VALUE='%s'\n", oid_str, type_str, value_str));

      /* Extract the oid from the string */
      size_t oid_len;
      oid *oid_array = oid_from_string(oid_str, &oid_len);

      /* Create a value struct from the strings */
      m_value *val = value_from_string(type_str, value_str);
      if (val)
      {
        /* Add the val to the values hashtable */
        i_hashtable_key *key = m_value_key(oid_str);
        i_hashtable_put(m_value_ht(), key, val);
        i_hashtable_key_free(key);

        /* Add SNMP Handler for the OID */
        DEBUGIF("mibjig:parse") {
            netsnmp_variable_list l;
            l.next_variable = NULL;
            l.name = oid_array;
            l.name_length = oid_len;
            l.type = val->type;
            l.val.string = (u_char *)val->val;      // string is just a member of the union
            l.val_len = val->val_len;
            DEBUGMSGTL(("mibjig:parse", "%s = %s: %s parsed to ", oid_str, type_str, value_str));
            DEBUGMSGVAR(("mibjig:parse", &l));
            DEBUGMSG_NC(("mibjig:parse", "\n"));
        }
        netsnmp_register_instance(
          netsnmp_create_handler_registration(oid_str, m_value_snmp_handler,
                                                   oid_array, oid_len, HANDLER_CAN_RONLY));
      }
    }
    else
    {
      /* Other matching error occurred */
      snmp_log(LOG_ERR, "m_parse other matching error occurred\n");
      pcre_free(re);    
      return;
    }
  }

  /* Clean up */
  pcre_free(re);




  /* Clean up */
  munmap(source, filesize);
  close(fd);


}
