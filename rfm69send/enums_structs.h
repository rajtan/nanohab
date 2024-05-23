
enum _SENS_TYPE {
  BINARY_SENSOR,
  ANALOG_SENSOR,
  BATT_SENSOR,
  ELECT_SENSOR,
  WATER_LEVEL_SENSOR,
};

enum _SENS_SUB_TYPE {
  MOTION_SENSOR,
  TEMP_SENSOR,
  DOOR_SENSOR,
  CURRENT_SENSOR,
  PH_SENSOR,
}
;
enum _VAL_TYPE {
  VT_NULL,
  VT_ONOFF,
  VT_TRIGGER,
  VT_INTEGER,
  VT_FLOAT,
};

enum _MSG_DIRN {
  NORTH,
  SOUTH,
};

enum _MSG_TYPE {
  NORMAL,
  CONTROL,
  TEST,
};

struct _SENS_INFO {
  uint8_t                 s_num;
  _SENS_TYPE              s_type;
  _SENS_SUB_TYPE          s_subtype;
  float                   s_val;
  _VAL_TYPE               s_val_type;
};

struct _MSG_STRUCT {
  uint32_t 	    message_seq;
  uint8_t       node_orig;
  uint8_t       node_dest;
  uint8_t		    msg_type;
  uint8_t		    msg_dirn;
  uint8_t		    sens_no;
  uint16_t		  sens_type;
  uint8_t		    sens_subtype;
  float		      sens_val;
  uint8_t		    sens_val_type;
};

