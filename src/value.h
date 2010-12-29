typedef struct m_value_s
{
  int type;
  void *val;
  size_t val_len;
} m_value;

struct i_hashtable_s* m_value_ht ();
m_value* m_value_create();
void m_value_free(m_value *val);
struct i_hashtable_key_s *m_value_key (char *oid_str);
int m_value_snmp_handler ();
