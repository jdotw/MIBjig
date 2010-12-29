#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <net-snmp/net-snmp-config.h>
#ifdef HAVE_SIGNAL
#include <signal.h>
#endif /* HAVE_SIGNAL */
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

#include "hashtable.h"
#include "value.h"

/*
 * Values Hashtable 
 *
 * This hashtable contains the values read from the SNMP walk output 
 * as m_value structs keyed by the handler string (the OID string) 
 */

#define VALUES_HT_SIZE 20000

static i_hashtable *static_values_ht = NULL;

i_hashtable* m_value_ht ()
{
  if (!static_values_ht) 
  { static_values_ht = i_hashtable_create(VALUES_HT_SIZE); }
  return static_values_ht; 
}

/* 
 * Struct Manipulation 
 */

m_value* m_value_create()
{
  m_value *val = (m_value *) malloc (sizeof(m_value));
  memset(val, 0, sizeof(m_value));
  return val;
}

void m_value_free(m_value *val)
{
  if (val->val) free (val->val);
  free (val);
}

/* 
 * Hash Key Creation 
 */

i_hashtable_key *m_value_key (char *oid_str)
{
  return i_hashtable_create_key_string(oid_str, VALUES_HT_SIZE);
}

/*
 * SNMP Handler 
 */

int m_value_snmp_handler (netsnmp_mib_handler *handler,
                          netsnmp_handler_registration *reginfo,
                          netsnmp_agent_request_info   *reqinfo,
                          netsnmp_request_info         *requests)
{
  i_hashtable_key *key = NULL;
  m_value *val = NULL;
  switch(reqinfo->mode)
  {
    case MODE_GET:
      key = m_value_key(reginfo->handlerName);
      val = (m_value *) i_hashtable_get(m_value_ht(), key);
      if (val)
      {
        snmp_set_var_typed_value(requests->requestvb, val->type, val->val, val->val_len);
      }
      else
      {
        printf("m_value_snmp_handler got no value for %s\n", reginfo->handlerName);
      }
      break;
    default:
      /* we should never get here, so this is a really bad error */
      snmp_log(LOG_ERR, "unknown mode (%d) in handle_ram\n", reqinfo->mode );
      return SNMP_ERR_GENERR;
  }

  return SNMP_ERR_NOERROR;
}

