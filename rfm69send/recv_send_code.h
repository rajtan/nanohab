
// void readOtherSensor(uint8_t sens_no=2){
//   // do processing for this sensor 
//   // do sens activation, delay (if required), read GPIO pins, 
//   // do computation / logi etc and finally
//   // sens_list[sens_no] = <value
// }

uint32_t static msg_seq=0;
struct _MSG_STRUCT msgbuf;
struct _MSG_STRUCT *p_msgbuf = &msgbuf;


void readMotion(uint8_t sens_no=1){
  sens_list[sens_no].s_val = 1.0;
};

void mkSensMessage(struct _MSG_STRUCT *p_msgbuf, uint8_t destnode, uint8_t sens_no) {
  p_msgbuf->node_orig     = pGC->nodeid;
  p_msgbuf->node_dest     = destnode;
  p_msgbuf->msg_type      = NORMAL;
  p_msgbuf->msg_dirn      = NORTH;
  p_msgbuf->sens_no       = sens_list[sens_no].s_num;
  p_msgbuf->sens_type     = sens_list[sens_no].s_type;
  p_msgbuf->sens_subtype  = sens_list[sens_no].s_subtype;
  p_msgbuf->sens_val      = sens_list[sens_no].s_val;
  p_msgbuf->sens_subtype  = sens_list[sens_no].s_val_type;
}

void sendMsgMotion(){
    readMotion( 1 );
    mkSensMessage(p_msgbuf, GW_NODEID, 1);
//PRE-CHECK RADIO
    radio.sendWithRetry(GW_NODEID, p_msgbuf, sizeof(msgbuf));
}

//PRE-CHECK RADIO - may completely comment recvProc

void recvProc(){
// //PRE-CHECK RADIO
//   if (radio.receiveDone())
//   {
//     uint8_t senderId;
//     int16_t rssi;
//     uint8_t data[RF69_MAX_DATA_LEN];

//     //save packet because it may be overwritten
// //PRE-CHECK RADIO x 3
//     senderId = radio.SENDERID;
//     rssi = radio.RSSI;
//     memcpy(data, (void *)radio.DATA, radio.DATALEN);
//     //check if sender wanted an ACK
//     if (radio.ACKRequested())
//     {
//       radio.sendACK();
//     }
//     radio.receiveDone(); //put radio in RX mode
//     //procRecvMsg(data, senderId);
//     //p_recvMsg = (struct _MSG_STRUCT *) data ;
//     //sprintf("org node=%3d : dest_node=%3d : msg_typ=%2d : msg_dirn=%d : sens_no=%3d : sens_type=%3d : sens_subtype=%3d : sens val=%8f : sens_valtype=%d\n",
//     //  p_recvMsg->node_orig,  p_recvMsg->node_dest, p_recvMsg->msg_type, p_recvMsg->msg_dirn, 
//     //  p_recvMsg->sens_no, p_recvMsg->sens_type, p_recvMsg->sens_subtype, p_recvMsg->sens_val, p_recvMsg->sens_subtype
//     //);
//   } else {
//     radio.receiveDone(); //put radio in RX mode
//   }
}


///////////////////////////////////////////////////////////////////////////
//
void radio_gw_setup() {
  int freq;
  static const char PROGMEM JSONtemplate[] =
    R"({"msgType":"config","freq":%d,"rfm69hcw":%d,"netid":%d,"mynodeid":%d,"power":%d})";
  char payload[150];
 
  //PRE-CHECK RADIO
  //radio = RFM69(RFM69_CS, RFM69_IRQ, GC_IS_RFM69HCW, RFM69_IRQN);
  Serial.printf("RFM69 Pin Config:\n");
  Serial.printf("RFM69_CS=%d, RFM69_IRQ=%d, GC_IS_RFM69HCW=%d, RFM69_IRQN=%d \n", RFM69_CS, RFM69_IRQ, GC_IS_RFM69HCW, RFM69_IRQN);

  // Hard Reset the RFM module
  pinMode(RFM69_RST, OUTPUT);
  digitalWrite(RFM69_RST, HIGH);
  delay(100);
  digitalWrite(RFM69_RST, LOW);
  delay(100);

  // Initialize radio
  //PRE-CHECK RADIO
  //radio.initialize(pGC->rfmfrequency, pGC->nodeid, pGC->networkid);
  Serial.printf("rfmfreq=%d, nodeid=%d, networkid=%d \n",pGC->rfmfrequency, pGC->nodeid, pGC->networkid );
  if (GC_IS_RFM69HCW) {
  //PRE-CHECK RADIO
    //radio.setHighPower();    // Only for RFM69HCW & HW!
    Serial.println("HighPower");
  }
  //PRE-CHECK RADIO
  //radio.setPowerLevel(GC_POWER_LEVEL); // power output ranges from 0 (5dBm) to 31 (20dBm)
  Serial.printf("Power level = %d\n",GC_POWER_LEVEL);
  //PRE-CHECK RADIO
  //if (pGC->encryptkey[0] != '\0') radio.encrypt(pGC->encryptkey);
  Serial.print(" ENc Key : "); Serial.println(pGC->encryptkey);
  pinMode(LED, OUTPUT);

  Serial.print("\nListening/Sending at ");
  switch (pGC->rfmfrequency) {
    case RF69_433MHZ:
      freq = 433;
      break;
    case RF69_868MHZ:
      freq = 868;
      break;
    case RF69_915MHZ:
      freq = 915;
      break;
    case RF69_315MHZ:
      freq = 315;
      break;
    default:
      freq = -1;
      break;
  }
  Serial.print(freq); Serial.print(' ');
  Serial.print(pGC->rfmfrequency); Serial.println(" MHz");

  size_t len = snprintf_P(RadioConfig, sizeof(RadioConfig), JSONtemplate,
      freq, GC_IS_RFM69HCW, pGC->networkid, pGC->nodeid, GC_POWER_LEVEL);
  if (len >= sizeof(RadioConfig)) {
    Serial.println("\n\n*** RFM69 config truncated ***\n");
  }
}

//////////////////////////////////////////////////////////////////////////
// Radio Gateway loop
void radio_gw_loop(){
    recvProc();
}

//////////////////////////////////////////////////////////////////////////
// flooding sending messaages for testing
void test_multimsg_send(uint8_t destnode) {
  int loops;
  uint32_t startMillis;
  static uint32_t deltaMillis = 0;

  delay(1000);  // Wait 1 second between transmits, could also 'sleep' here!

  char radioPerf[32];
  ultoa(packetnum++, radioPerf, 10);
  strcat(radioPerf, ",");
  ltoa(radio.readRSSI(false), radioPerf+strlen(radioPerf), 10);
  strcat(radioPerf, ",");
  ultoa(deltaMillis, radioPerf+strlen(radioPerf), 10);
  Serial.print("Sending "); Serial.print(radioPerf); Serial.print(' ');

  loops = 10;
  startMillis = millis();
  while (loops--) {
    if (radio.sendWithRetry(destnode, radioPerf, strlen(radioPerf)+1)) {
      deltaMillis = millis() - startMillis;
      Serial.print(" OK ");
      Serial.println(deltaMillis);
      break;
    }
    else {
      Serial.print("!");
    }
    delay(50);
  }
  if (loops <= 0) {
    Serial.println(" Fail");
    deltaMillis = 0;
  }

  radio.receiveDone(); //put radio in RX mode
}

