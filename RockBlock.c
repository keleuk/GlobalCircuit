#include <stdint.h>
#include "RockBlock.h"
#include "UART.h"
#include "Yikes.h"
#include "Ballast.h"
#include "Cutdown.h"

char _rb_reqsend;        //flag indicating that a request to send data was made
uint8_t _rb_reqSigCheck; //flag indicating that a request to check signal strength was made
uint16_t _rb_idletimer;  //timer that keeps track of number of ticks RB has been busy consecutively

void ParseSBDIX(volatile char *cmdbuf,char *mos,char *mom,char *mts,char *mtm,char *mtl,char *mtq);
uint8_t ParseCSQ(volatile char *cmdbuf);
uint16_t ParseSN(volatile char *cmdbuf);

void InitRB() {
    _rb_seq = RB_INIT;    //set rockblock to initialization state
    _rb_status = RB_OK;
    _rb_command_ind = 0;  
    _rb_idx = 0;          //set rockblock character index to 0
    _rb_cmdbuf[340]=0;    //clear the rockblock command buffer
    _rb_reqsend=0;        //clear reqsend flag
    _rb_reqSigCheck=0;    //clear signal check flag
    _rb_idletimer=0;      //reset idle timer
}

rb_command_resp_t RB_GetSerial()
{
    if(_rb_status == RB_OK)
    {
        SendString_UART1("AT+CGSN");
        _rb_status = RB_BUSY;
        return RB_COMMAND_NEXT;
    }
    if(_rb_status == RB_ERROR)
    {
        yikes.rberror=1;
        return RB_COMMAND_RESET;
    }
    return RB_COMMAND_HOLD;
}

rb_command_resp_t RB_Echo_Off()
{
    if(_rb_status == RB_OK)
    {
        SendString_UART1("ATE0");
        _rb_status = RB_BUSY;
        return RB_COMMAND_NEXT;
    }
    if(_rb_status == RB_ERROR)
    {
        yikes.rberror=1;
        return RB_COMMAND_RESET;
    }
    return RB_COMMAND_HOLD;
    
}

rb_command_resp_t RB_Echo_On()
{
    if(_rb_status == RB_OK)
    {
        SendString_UART1("ATE1");
        _rb_status = RB_COMMAND_NEXT;
        return 1;
    }
    else if(_rb_status == RB_ERROR)
    {
        yikes.rberror=1;
        return RB_COMMAND_RESET;
    }
    return RB_COMMAND_HOLD;
    
}

rb_command_resp_t RB_FlowControl_Disable()
{
    if(_rb_status == RB_OK)
    {
        SendString_UART1("AT&K0\r");
        _rb_status = RB_BUSY;
        return RB_COMMAND_NEXT;
    }
    if(_rb_status == RB_ERROR)
    {
        yikes.rberror=1;
        return RB_COMMAND_RESET;
    }
    return RB_COMMAND_HOLD;
}

rb_command_resp_t RB_FlowControl_Enable()
{
    if(_rb_status == RB_OK)
    {
        SendString_UART1("AT&K3\r");
        _rb_status = RB_BUSY;
        return RB_COMMAND_NEXT;
    }
    if(_rb_status == RB_ERROR)
    {
        yikes.rberror=1;
        return RB_COMMAND_RESET;
    }
    return RB_COMMAND_HOLD;
}

rb_command_resp_t RB_DTR_Ignore()
{
    if(_rb_status == RB_OK)
    {
        SendString_UART1("AT&D0\r");
        _rb_status = RB_BUSY;
        return RB_COMMAND_NEXT;
    }
    if(_rb_status == RB_ERROR)
    {
        yikes.rberror=1;
        return RB_COMMAND_RESET;
    }
    return RB_COMMAND_HOLD;
    
}

rb_command_resp_t RB_DTR_10Sec()
{
    if(_rb_status == RB_OK)
    {
        SendString_UART1("AT&D1\r");
        _rb_status = RB_BUSY;
        return RB_COMMAND_NEXT;
    }
    if(_rb_status == RB_ERROR)
    {
        yikes.rberror=1;
        return RB_COMMAND_RESET;
    }
    return RB_COMMAND_HOLD;
}

rb_command_resp_t RB_DTR_Def()
{
    if(_rb_status == RB_OK)
    {
        SendString_UART1("AT&D2\r");
        _rb_status = RB_BUSY;
        return RB_COMMAND_NEXT;
    }
    if(_rb_status == RB_ERROR)
    {
        yikes.rberror=1;
        return RB_COMMAND_RESET;
    }
    return RB_COMMAND_HOLD;
}

rb_command_resp_t RB_DTR_Reset()
{
    if(_rb_status == RB_OK)
    {
        SendString_UART1("AT&D3\r");
        _rb_status = RB_BUSY;
        return RB_COMMAND_NEXT;
    }
    if(_rb_status == RB_ERROR)
    {
        yikes.rberror=1;
        return RB_COMMAND_RESET;
    }
    return RB_COMMAND_HOLD;
}

rb_command_resp_t RB_Ring_Disable()
{
    if(_rb_status == RB_OK)
    {
        SendString_UART1("AT+SBDMTA=0\r");
        _rb_status = RB_BUSY;
        return RB_COMMAND_NEXT;
    }
    if(_rb_status == RB_ERROR)
    {
        yikes.rberror=1;
        return RB_COMMAND_RESET;
    }
    return RB_COMMAND_HOLD;
}

rb_command_resp_t RB_Ring_Enable()
{
    if(_rb_status == RB_OK)
    {
        SendString_UART1("AT+SBDMTA=1\r");
        _rb_status = RB_BUSY;
        return RB_COMMAND_NEXT;
    }
    if(_rb_status == RB_ERROR)
    {
        yikes.rberror=1;
        return RB_COMMAND_RESET;
    }
    return RB_COMMAND_HOLD;
}

rb_command_resp_t RB_Clear_TxBuff()
{
    if(_rb_status == RB_OK)
    {
        SendString_UART1("AT+SBDD0\r");
        _rb_status = RB_BUSY;
        return RB_COMMAND_NEXT;
    }
    if(_rb_status == RB_ERROR)
    {
        yikes.rberror=1;
        return RB_COMMAND_RESET;
    }
    return RB_COMMAND_HOLD;
}

rb_command_resp_t RB_Clear_RxBuff()
{
    if(_rb_status == RB_OK)
    {
        SendString_UART1("AT+SBDD1\r");
        _rb_status = RB_BUSY;
        return RB_COMMAND_NEXT;
    }
    if(_rb_status == RB_ERROR)
    {
        yikes.rberror=1;
        return RB_COMMAND_RESET;
    }
    return RB_COMMAND_HOLD;
}

rb_command_resp_t RB_Clear_BothBuff()
{
    if(_rb_status == RB_OK)
    {
        SendString_UART1("AT+SBDD2\r");
        _rb_status = RB_BUSY;
        return RB_COMMAND_NEXT;
    }
    if(_rb_status == RB_ERROR)
    {
        yikes.rberror=1;
        return RB_COMMAND_RESET;
    }
    return RB_COMMAND_HOLD;
}

rb_command_resp_t RB_CheckSig() 
{
    if(_rb_status == RB_OK)
    {
        SendString_UART1("AT+CSQ\r");
        _rb_status = RB_BUSY;
        return RB_COMMAND_NEXT;
    }
    if(_rb_status == RB_ERROR)
    {
        yikes.rberror=1;
        return RB_COMMAND_RESET;
    }
    return RB_COMMAND_HOLD;
}

rb_command_resp_t RB_Rx()
{
    if(_rb_status == RB_OK)
    {
        ParseSBDIX(_rb_cmdbuf,_rb_mos,_rb_mom,_rb_mts,_rb_mtm,_rb_mtl,_rb_mtq);
        _rb_imtl=atoi(_rb_mtl);
        if (_rb_imtl>0) 
        {
            SendString_UART1("AT+SBDRB\r");
            _rb_status = RB_BUSY;
            _rb_seq = RB_UPLINK;
        }
        return RB_COMMAND_NEXT;
    }
    if(_rb_status == RB_ERROR)
    {
        yikes.rberror = 1;
        return RB_COMMAND_RESET;
    }
    return RB_COMMAND_HOLD;
}

rb_command_resp_t RB_WriteBuff()
{
    if(_rb_status == RB_OK)
    {
        SendString_UART1("AT+SBDWB=340\r");
        _rb_status = RB_BUSY;
    }
    else if(_rb_status == RB_READY)
    {
        uint16_t csum = 0;
        uint16_t i;
        SendBuffer_UART1((char *)RBTXbuf, 0, 340);
        for(i=0;i<340;i++)
            csum+=(uint8_t) RBTXbuf[i];
        SendChar_UART1(csum>>8);
        SendChar_UART1(csum&0xFF);
        _rb_status = RB_BUSY;
        return RB_COMMAND_NEXT;
    }
    else if(_rb_status == RB_ERROR)
    {
        yikes.rberror = 1;
        return RB_COMMAND_RESET;
    }
    return RB_COMMAND_HOLD;
}

rb_command_resp_t RB_Tx()
{
    if(_rb_status == RB_OK)
    {
        SendString_UART1("AT+SBDI\r");
        _rb_status = RB_BUSY;
        return RB_COMMAND_NEXT;
    }
    if(_rb_status == RB_ERROR)
    {
        yikes.rberror=1;
        return RB_COMMAND_RESET;
    }
    return RB_COMMAND_HOLD;
}

rb_command_resp_t RB_ParseUplink()
{
    if(_rb_status == RB_OK)
    {
        volatile char *rbuf=_rb_cmdbuf; //create an instance of the message buffer
        uint16_t msglen=*rbuf++<<8;     //pull the message length from the message buffer
        msglen|=*rbuf++;
        uint16_t i;
        for (i=0;i<msglen;i++) {        //loop through the message
            RBRXbuf[i]=*rbuf++;         //retrieve the message from the message buffer
        }
        RBRXbuf[msglen]=0;              //add a 0 terminator to the end of the message
        if(!ballast_rq && BALLAST && REQUEST)          //if a ballast request has come in
        {
            ballast_rq = 1;                            //set ballast_rq flag
        }
        else if(ballast_rq >= 3 && ballast_rq <= 12 && BALLAST && CONFIRM && ((RBRXbuf[8] >= '0' && RBRXbuf[8] <= '9') || (RBRXbuf[8] >= 'A' && RBRXbuf[8] <= 'F') || (RBRXbuf[8] >= 'a' && RBRXbuf[8] <= 'f'))) //if a ballast acknowledge is expected and was received
        {
            if(RBRXbuf[8] <= '9')
                DeployBallast(RBRXbuf[8] - '0');                          //begin ballast deployment
            else if (RBRXbuf[8] <= 'F')
                DeployBallast(RBRXbuf[8] - 'A' + 10);
            else
                DeployBallast(RBRXbuf[8] - 'a' + 10);
            ballast_rq = 0;                            //clear ballast_rq flag
        }
        else if(!cutdown_rq && CUTDOWN && REQUEST)     //if a cutdown request has come in
        {
            cutdown_rq = 1;                            //set cutdown_rq flag
        }
        else if(cutdown_rq >= 3 && cutdown_rq <= 12 && CUTDOWN && CONFIRM) //if a cutdown acknowledge is expected as was received
        {
             InitiateCutdown();                        //initiate a cutdown sequence
             cutdown_rq = 0;                           //clear the cutdown_rq flag
        }
        return RB_COMMAND_NEXT;
    }
    if(_rb_status == RB_ERROR)
    {
        yikes.rberror = 1;
        return RB_COMMAND_RESET;
    }
    return RB_COMMAND_HOLD;
}

void TickRB()
{
    switch(_rb_seq)
    {
        case RB_INIT:
            if(_rb_init_funcs[_rb_command_ind])
            {
                switch((*_rb_init_funcs[_rb_command_ind])())
                {
                    case RB_COMMAND_NEXT:
                        ++_rb_command_ind;
                        break;
                        
                    case RB_COMMAND_RESET:
                        _rb_command_ind = 0;
                        break;
                        
                    case RB_COMMAND_HOLD:
                        break;
                }
            }
            else
            {
                _rb_command_ind = 0;
                _rb_seq = RB_IDLE;
            }
            break;
            
        case RB_TRANS:
            if(_rb_trans_funcs[_rb_command_ind])
            {
                switch((*_rb_trans_funcs[_rb_command_ind])())
                {
                    case RB_COMMAND_NEXT:
                        ++_rb_command_ind;
                        break;
                        
                    case RB_COMMAND_RESET:
                        // while(1); RESET whole system?
                        _rb_command_ind = 0;
                        break;
                        
                    case RB_COMMAND_HOLD:
                        break;
                }
            }
            else
            {
                _rb_command_ind = 0;
                _rb_seq = RB_IDLE;
            }
            break;
            
        case RB_UPLINK:
            switch(RB_ParseUplink())
            {
                case RB_COMMAND_NEXT:
                    _rb_command_ind = 0;
                    _rb_seq = RB_IDLE;
                    break;
                    
                case RB_COMMAND_RESET:
                    _rb_command_ind = 0;
                    _rb_seq = RB_IDLE;
                    break;
                    
                case RB_COMMAND_HOLD:
                    break;
            }
            break;
            
        case RB_IDLE:
            break;
    }
}

void TickRB() {
    uint16_t csum=0; //checksum 
    uint16_t i;      //looping variable
    if (_rb_state==RB_IDLE) //if the rockblock is idle,
        _rb_idletimer=0;    //reset the idle timer
    else                    //if the rockblock is busy,
        _rb_idletimer++;    //increment the idle timer
    
    if (_rb_idletimer==RB_IDLE_SOFT_TIMEOUT) { //if the rockblock has reached a soft timeout,
        _rb_state=RB_INIT;                     //reinitialize communication
        yikes.rbtime=1;                        //set the rbtime yikes flag
    }
    if (_rb_idletimer==RB_IDLE_FIRM_TIMEOUT) { //if the rockblock has reached a firm timeout,
        _rb_state=RB_INIT;                     //reinitialize communication
        yikes.rbtime=1;                        //set the rbtime yikes flag
    }
    if (_rb_idletimer==RB_IDLE_HARD_TIMEOUT) { //if the rockblock has reached a hard timeout
        while(1);                              //wait until the watchdog resets the PIC
    }
    switch (_rb_state){
    /////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////INITIALIZATION//////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////
        case RB_INIT:                          //if in initialization state,
            SendString_UART1("ATE0\r");        //begin AT command mode
            _rb_state=SENT_ATEo;               //update state to SENT_ATEo
            _rb_status=RB_BUSY;                //indicate that the rockblock is busy
            break;
        case SENT_ATEo:                        //if in AT command mode,
            if (_rb_status==RB_OK) {           //if rockblock is receiving commands correctly
                SendString_UART1(RB_NO_FLOWC); //set no flow control
                _rb_state=SENT_ATnKo;          //update state to SENT_ATnKo;
                _rb_status=RB_BUSY;            //indicate that the rockblock is busy
            }
            if (_rb_status==RB_ERROR) {        //if rockblock received an error,
                yikes.rberror=1;               //set the rberror yikes flag
                _rb_state=RB_INIT;             //reinitialize the communication
            }
            break;
        case SENT_ATnKo:
            if (_rb_status==RB_OK) {           //if rockblock is receiving commands correctly
                SendString_UART1(RB_NO_DTR);   //set no flow control
                _rb_state=SENT_ATnDo;          //update state to SENT_ATnKo;
                _rb_status=RB_BUSY;            //indicate that the rockblock is busy
            }
            if (_rb_status==RB_ERROR) {        //if rockblock received an error,
                yikes.rberror=1;               //set the rberror yikes flag
                _rb_state=RB_INIT;             //reinitialize the communication
            }
            break;
        case SENT_ATnDo:                       //if no flow control was set,
            if (_rb_status==RB_OK) {           //if rockblock is recieving commands correctly,
                SendString_UART1(RB_NO_RINGID);//disable ring identification
                _rb_state=SENT_SBDMTA;         //update state to SENT_SBDMTA
                _rb_status=RB_BUSY;            //indicate that the rockblock is busy
            }
            if (_rb_status==RB_ERROR) {        //if the rockblock received an error,
                yikes.rberror=1;               //set the rberror yikes flag
                _rb_state=RB_INIT;             //reinitialize the communication
            }
            break;
        case SENT_SBDMTA:                       //if ring identification has been disabled,
            if (_rb_status==RB_OK) {            //if the rockblock is receiving commands correctly,
                SendString_UART1(RB_CLEARBUFF); //clear the message buffer
                _rb_state=SENT_SBDDo;           //update state to SEND_SBDDo
                _rb_status=RB_BUSY;             //indicate that the rockblock is busy
            }
            if (_rb_status==RB_ERROR) {         //if the rockblock received an error
                yikes.rberror=1;                //set the rberror yikes flag
                _rb_state=RB_INIT;              //reinitialize the communication
            }
            break;
        case SENT_SBDDo:                        //if the message buffer has been cleared,
            if (_rb_status==RB_OK) {            //if the rockblock is receiving commands correctly,
                SendString_UART1(RB_READIMEI);  //clear the message buffer
                _rb_state=SENT_CGSN;
                _rb_status=RB_BUSY;
            }
            if (_rb_status==RB_ERROR) {         //if the rockblock received an error,
                yikes.rberror=1;                //set the rberror yikes flag
                _rb_state=RB_INIT;              //reinitialize communication
            }
            break;
        case SENT_CGSN:                         //if the IMEI has been requested
            if (_rb_status==RB_OK) {            //if the rockblock is receiving commands correctly,
                _rb_imei = ParseSN(_rb_cmdbuf); //read and store the last 4 digits of the serial number
                _rb_state=RB_IDLE;              //set rockblock to idle
            }
            if(_rb_status==RB_ERROR) {          //if the rockblock received an error,
                yikes.rberror=1;                //set te rberror yikes flag
                _rb_state=RB_INIT;              //reinitialize communication
            }
            break;
    //////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////SIGNAL STRENGTH CHECK///////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////
        case CHECK_SIG:                         //if checking rockblock signal
            SendString_UART1(RB_READSIG);       //ask the rockblock for signal strength
            _rb_state=SENT_CSQ;                 //set state to sent CSQ
            _rb_status=RB_BUSY;                 //indicate that the rockblock is busy
            break;
        case SENT_CSQ:                          //if signal strength has been requested
            if (_rb_status==RB_OK) {            //if rockblock is receiving commands correctly,
                _rb_state=RB_IDLE;              //set rockblock to idle state
                _rb_sig = ParseCSQ(_rb_cmdbuf); //parse the CSQ message and store the signal strength
            }
            if (_rb_status==RB_ERROR) {         //if the rockblock received an error,
                yikes.rberror=1;                //set the rberror yikes flag
                _rb_state=RB_IDLE;              //reinitialize communication
            }
            break;
    ///////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////IDLE////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////
        case RB_IDLE:                //if the rockblock is idle,
            if (_rb_reqsend)         //if a request to send has been administered,
            {
                _rb_reqsend=0;       //clear reqsend flag
                _rb_state=BEGINSEND; //update state to check signal
            }
            else if(_rb_reqSigCheck) //if a request to check signal has been administered,
            {
                _rb_reqSigCheck=0;   //clear signal request flag
                _rb_state=CHECK_SIG; //set rockblock state to check the signal
            }
            break;
    ///////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////SENDING MESSAGE///////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////
        case BEGINSEND:                   //if it is time to send a message,
            SendString_UART1("AT\r");//prepare a message of 340 characters maximum
            _rb_state=S1;         //update state to SENT_SBDWB
            _rb_status=RB_BUSY;           //indicate that the rockblock is busy
            break;
        case S1:
            if(_rb_status==RB_OK) {
                SendString_UART1(RB_NO_FLOWC);
                _rb_state=S2;
                _rb_status=RB_BUSY;
            }
            if(_rb_status==RB_ERROR) {
                yikes.rberror=1;          //set the rberror yikes flag
                _rb_state=RB_IDLE;        //reinitialize the communication
            }
            break;
        case S2:
            if(_rb_status==RB_OK) {
                SendString_UART1(RB_WRITE340);
                _rb_state = SENT_SBDWB;
                _rb_status = RB_BUSY;
            }
        case SENT_SBDWB:                  //if a message has been prepared,
            if (_rb_status==RB_READY) {   //if the rockblock is ready for the message,
                _rb_state=SENDING_TXBUF;  //update state to SENDING_TXBUF
                _rb_buf_sindex=0;         //reset the rockblock buffer index
            }
            if (_rb_status==RB_ERROR) {   //if the rockblock received an error,
                yikes.rberror=1;          //set the rberror yikes flag
                _rb_state=RB_IDLE;        //reinitialize the communication
            }
            break;
        case SENDING_TXBUF:               //if we are ready to send the message,
            if (_rb_buf_sindex<10) {      //if there is still message left to be sent,
                SendBuffer_UART1((char *)RBTXbuf,_rb_buf_sindex*34,_rb_buf_sindex*34+34); //send a tenth of the message
                _rb_buf_sindex++;         //increment buffer index counter
            } else {                      //if the full message has been sent,
                _rb_state=SENT_TXBUF;     //update state to SENT_TXBUF
            }
            break;
        case SENT_TXBUF:                    //if the full message has been sent,
            for (i=0;i<340;i++)             //loop through the full message
                csum+=(uint8_t) RBTXbuf[i]; //calculate the checksum
            SendChar_UART1(csum>>8);        //send the most significant half of the checksum
            SendChar_UART1(csum&0xFF);      //send the least significant half of the checksum
            _rb_state=SENT_CSUM;            //update state to SENT_CSUM
            _rb_status=RB_BUSY;             //indicate that the rockblock is busy
            break;
        case SENT_CSUM:                         //if the checksum has been sent,
            if (_rb_status==RB_OK) {            //if the rockblock is receiving commands correctly,
                SendString_UART1(RB_SEND);      //initiate an SBD session
                _rb_state=SENT_SBDIX;           //update state to SENT_SBDIX
                _rb_status=RB_BUSY;             //indicate that the rockblock is busy
            }
            if (_rb_status==RB_ERROR) {         //if the rockblock received an error
                yikes.rberror=1;                //set the rberror yikes flag
                _rb_state=RB_IDLE;              //reinitialize the communication
            }
            break;
        case SENT_SBDIX:                            //if the message was sent,
            if (_rb_status==RB_OK) {                //if the rockblock is receiving commands correctly,
                                                    //parse the rockblock's status response to receiving the message
                ParseSBDIX(_rb_cmdbuf,_rb_mos,_rb_mom,_rb_mts,_rb_mtm,_rb_mtl,_rb_mtq);
                _rb_imtl=atoi(_rb_mtl);             //parse the message length parameter from ascii to integer
                if (_rb_imtl>0) {                   //if the message has a length greater than 0
                    SendString_UART1(RB_RECEIVE);   //send the message
                    _rb_state=SENT_SBDRB;           //update state to SENT_SBDRB
                    _rb_status=RB_BUSY;             //indicate that the rockblock is busy
                } else {                            //if there is no message
                    SendString_UART1(RB_CLEARBUFF);
                    _rb_state=S3;       //set the rockblock to idle status
                    _rb_status=RB_BUSY;
                }
            }
            if (_rb_status==RB_ERROR) {             //if the rockblock received an error,
                yikes.rberror=1;                    //set the rberror yikes flag
                _rb_state=RB_IDLE;                  //reinitialize the communication
            }
            break;
        case SENT_ACKAT:                 //if no message was sent,
            if (_rb_status==RB_OK) {     //if the rockblock is receiving commands correctly,
                SendString_UART1(RB_CLEARBUFF);
                _rb_state=S3;       //set the rockblock to idle status
                _rb_status=RB_BUSY;
            }
            if (_rb_status==RB_ERROR) {  //if the rockblock received an error,
                yikes.rberror=1;         //set the rberror yikes flag
                _rb_state=RB_IDLE;       //reinitialize the communication
            }
            break;
        case S3:
            if (_rb_status==RB_OK) {
                _rb_state=RB_IDLE;
            }
            if (_rb_status==RB_ERROR) {
                yikes.rberror=1;
                _rb_state=RB_IDLE;
            }
            break;
        case SENT_SBDRB:                        //if a message was sent,
            if (_rb_status==RB_OK) {            //if the rockblock is receiving commands correctly,
                ++command_count;                //increment command counter
                volatile char *rbuf=_rb_cmdbuf; //create an instance of the message buffer
                uint16_t msglen=*rbuf++<<8;     //pull the message length from the message buffer
                msglen|=*rbuf++;
                for (i=0;i<msglen;i++) {        //loop through the message
                    RBRXbuf[i]=*rbuf++;         //retrieve the message from the message buffer
                }
                RBRXbuf[msglen]=0;              //add a 0 terminator to the end of the message
                if(!ballast_rq && BALLAST && REQUEST)          //if a ballast request has come in
                {
                    ballast_rq = 1;                            //set ballast_rq flag
                    _rb_state=RB_IDLE;                         //rockblock is now idle
                }
                else if(ballast_rq >= 3 && ballast_rq <= 12 && BALLAST && CONFIRM && ((RBRXbuf[8] >= '0' && RBRXbuf[8] <= '9') || (RBRXbuf[8] >= 'A' && RBRXbuf[8] <= 'F') || (RBRXbuf[8] >= 'a' && RBRXbuf[8] <= 'f'))) //if a ballast acknowledge is expected and was received
                {
                    if(RBRXbuf[8] <= '9')
                        DeployBallast(RBRXbuf[8] - '0');                          //begin ballast deployment
                    else if (RBRXbuf[8] <= 'F')
                        DeployBallast(RBRXbuf[8] - 'A' + 10);
                    else
                        DeployBallast(RBRXbuf[8] - 'a' + 10);
                    ballast_rq = 0;                            //clear ballast_rq flag
                    _rb_state=RB_IDLE;                         //rockblock is now idle
                }
                else if(!cutdown_rq && CUTDOWN && REQUEST)     //if a cutdown request has come in
                {
                    cutdown_rq = 1;                            //set cutdown_rq flag
                    _rb_state=RB_IDLE;                         //rockblock is now idle
                }
                else if(cutdown_rq >= 3 && cutdown_rq <= 12 && CUTDOWN && CONFIRM) //if a cutdown acknowledge is expected as was received
                {
                     InitiateCutdown();                        //initiate a cutdown sequence
                     cutdown_rq = 0;                           //clear the cutdown_rq flag
                     _rb_state=RB_IDLE;                        //rockblock is now idle
                }
                else {                        //if none of the above messages were received
                    SendString_UART1("AT\r"); //send an empty AT command
                    _rb_state=SENT_ACKAT;     //update state to SENT ACKAT
                    _rb_status=RB_BUSY;       //indicate that the rockblock is busy
                }
            }
            if (_rb_status==RB_ERROR) { //if the rockblock received an error,
                yikes.rberror=1;        //set the rberror yikes flag
                _rb_state=RB_INIT;      //reinitialize the communication
            }
            break;
  }
}

void SendString_RB(char *msg) {
    memcpy((void *)RBTXbuf,msg,340); //copy the message into the transmitting buffer
    _rb_reqsend=1;                   //request to send the message
}

uint8_t ParseCSQ(volatile char *cmdbuf)
{
    while(*cmdbuf!=':')                        //move through the command buffer until a ':' is found
        ++cmdbuf;
    while(!(*cmdbuf >= '0' && *cmdbuf <= '9')) //move through the command buffer until a number is found
        ++cmdbuf;
        return (*cmdbuf) - '0';                //return the signal strength
}

uint16_t ParseSN(volatile char *cmdbuf)
{
    uint8_t i;                               //looping variable
    uint16_t scale = 1000;                   //scalar for adding to the SN
    uint16_t sn = 0;                         //serial number reading
    while(*cmdbuf<'0' || *cmdbuf>'9')        //loop through characters until a number is reached
        ++cmdbuf;
    for(i=0; i < 11; ++i)                    //loop through the next 11 characters
        ++cmdbuf;
    while(scale)                             //loop until 4 digits have been read
    {
        sn += (*cmdbuf++ - '0') * scale;
        scale /= 10;
    }
    return sn;                               //return the last 4 digits of the serial number
}

void ParseSBDIX(volatile char *cmdbuf,char *mos,char *mom,char *mts,char *mtm,char *mtl,char *mtq) {
    uint8_t field=0;       //counter for which field is currently being read in
    uint16_t fieldstart=0; //starting index of current field
    uint16_t idx=0;        //current index
    while (*cmdbuf!='+')   //loop until initiating character is reached
        cmdbuf++;          //move to the next character
    while (cmdbuf[idx]) {  //loop until terminating character is reached
        if (cmdbuf[idx]==',' || cmdbuf[idx]=='\r' || cmdbuf[idx]==':') { //if a separating character is read
            switch(field) {               
                case 1:                   //if we are looking at the mos field,
                    mos[idx-fieldstart]=0;//add a terminating character to the end of the mos field
                    break;
                case 2:                   //if we are looking at the mom field,
                    mom[idx-fieldstart]=0;//add a terminating character to the end of the mom field
                    break;
                case 3:                   //if we are looking at the mts field,
                    mts[idx-fieldstart]=0;//add a terminating character to the end of the mts field
                    break;
                case 4:                   //if we are looking at the mtm field,
                    mtm[idx-fieldstart]=0;//add a terminating character to the end of the mtm field
                    break;
                case 5:                   //if we are looking at the mtl field,
                    mtl[idx-fieldstart]=0;//add a terminating character to the end of the mtl field
                    break;
                case 6:                   //if we are looking at the mtq field,
                    mtq[idx-fieldstart]=0;//add a terminating character to the end of the mtq field
                    break;
            }
            field++;          //move to the next field
            fieldstart=idx+1; //update the fieldstart index
        } else {                                     //if a data character is read
            switch(field) {
                case 1:                              //if we are looking at the mos field,
                    mos[idx-fieldstart]=cmdbuf[idx]; //add the data character to the mos field
                    break;
                case 2:                              //if we are looking at the mom field,
                    mom[idx-fieldstart]=cmdbuf[idx]; //add the data character to the mom field
                    break;
                case 3:                              //if we are looking at the mts field,
                    mts[idx-fieldstart]=cmdbuf[idx]; //add the data character to the mts field
                    break;
                case 4:                              //if we are looking at the mtm field,
                    mtm[idx-fieldstart]=cmdbuf[idx]; //add the data character to the mtm field
                    break;
                case 5:                              //if we are looking at the mtl field,
                    mtl[idx-fieldstart]=cmdbuf[idx]; //add the data character to the mtl field
                    break;
                case 6:                              //if we are looking at the mtq field,
                    mtq[idx-fieldstart]=cmdbuf[idx]; //add the data character to the mtq field
                    break;
            }
        }
        idx++; //move to the next character
    }
}
