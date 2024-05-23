
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

//////////////////////////////////////////////////////////////////////////
// Gateway loop
void loop_gateway(){
    recvProc();
}
