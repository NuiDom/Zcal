
/* Includes ------------------------------------------------------------------*/

/* mbed specific header files. */
#include "mbed.h"
#include "QEI.h"
/* Helper header files. */
#include "DevSPI.h"
/* Component specific header files. */
#include "L6474.h"

char szCommand[100];
int targetPosition;
Serial pc(USBTX, USBRX);
QEI wheel (PA_0, PA_1, NC, 624);

/* Variables -----------------------------------------------------------------*/

/* Initialization parameters. */
L6474_init_t init = {
    160,                              /* Acceleration rate in pps^2. Range: (0..+inf). */
    160,                              /* Deceleration rate in pps^2. Range: (0..+inf). */
    800,                             /* Maximum speed in pps. Range: (30..10000]. */
    800,                              /* Minimum speed in pps. Range: [30..10000). */
    2200,                              /* Torque regulation current in mA. Range: 31.25mA to 4000mA. */
    L6474_OCD_TH_2625mA,               /* Overcurrent threshold (OCD_TH register). */
    L6474_CONFIG_OC_SD_ENABLE,        /* Overcurrent shutwdown (OC_SD field of CONFIG register). */
    L6474_CONFIG_EN_TQREG_TVAL_USED,  /* Torque regulation method (EN_TQREG field of CONFIG register). */
    L6474_STEP_SEL_1,              /* Step selection (STEP_SEL field of STEP_MODE register). */
    L6474_SYNC_SEL_1_2,               /* Sync selection (SYNC_SEL field of STEP_MODE register). */
    L6474_FAST_STEP_12us,             /* Fall time value (T_FAST field of T_FAST register). Range: 2us to 32us. */
    L6474_TOFF_FAST_8us,              /* Maximum fast decay time (T_OFF field of T_FAST register). Range: 2us to 32us. */
    3,                                /* Minimum ON time in us (TON_MIN register). Range: 0.5us to 64us. */
    21,                               /* Minimum OFF time in us (TOFF_MIN register). Range: 0.5us to 64us. */
    L6474_CONFIG_TOFF_044us,          /* Target Swicthing Period (field TOFF of CONFIG register). */
    L6474_CONFIG_SR_320V_us,          /* Slew rate (POW_SR field of CONFIG register). */
    L6474_CONFIG_INT_16MHZ,           /* Clock setting (OSC_CLK_SEL field of CONFIG register). */
    L6474_ALARM_EN_OVERCURRENT |
    L6474_ALARM_EN_THERMAL_SHUTDOWN |
    L6474_ALARM_EN_THERMAL_WARNING |
    L6474_ALARM_EN_UNDERVOLTAGE |
    L6474_ALARM_EN_SW_TURN_ON |
    L6474_ALARM_EN_WRONG_NPERF_CMD    /* Alarm (ALARM_EN register). */
};

/* Motor Control Component. */
L6474 *motor;


/* Functions -----------------------------------------------------------------*/


void Move();
void ReadCommand(void);
void InterpetCommand(void);

  
void FlagIRQHandler(void)
{
    /* Set ISR flag. */
    motor->isr_flag = TRUE;

    /* Get the value of the status register. */
    unsigned int status = motor->get_status();
    
    /* Check HIZ flag: if set, power brigdes are disabled. */
    if ((status & L6474_STATUS_HIZ) == L6474_STATUS_HIZ)
    { /* HIZ state. Action to be customized. */ }
    
    /* Check direction. */
    if ((status & L6474_STATUS_DIR) == L6474_STATUS_DIR)
    { /* Forward direction is set. Action to be customized. */ }
    else
    { /* Backward direction is set. Action to be customized. */ }
    
    /* Check NOTPERF_CMD flag: if set, the command received by SPI can't be performed. */
    /* This often occures when a command is sent to the L6474 while it is not in HiZ state. */
    if ((status & L6474_STATUS_NOTPERF_CMD) == L6474_STATUS_NOTPERF_CMD)
    { /* Command received by SPI can't be performed. Action to be customized. */ }
     
    /* Check WRONG_CMD flag: if set, the command does not exist. */
    if ((status & L6474_STATUS_WRONG_CMD) == L6474_STATUS_WRONG_CMD)
    { /* The command received by SPI does not exist. Action to be customized. */ }
    
    /* Check UVLO flag: if not set, there is an undervoltage lock-out. */
    if ((status & L6474_STATUS_UVLO) == 0)
    { /* Undervoltage lock-out. Action to be customized. */ }
    
    /* Check TH_WRN flag: if not set, the thermal warning threshold is reached. */
    if ((status & L6474_STATUS_TH_WRN) == 0)
    { /* Thermal warning threshold is reached. Action to be customized. */ }
    
    /* Check TH_SHD flag: if not set, the thermal shut down threshold is reached. */
    if ((status & L6474_STATUS_TH_SD) == 0)
    { /* Thermal shut down threshold is reached. Action to be customized. */ }
    
    /* Check OCD  flag: if not set, there is an overcurrent detection. */
    if ((status & L6474_STATUS_OCD) == 0)
    { /* Overcurrent detection. Action to be customized. */ }

    /* Reset ISR flag. */
    motor->isr_flag = FALSE;
}

/**
  * @brief  This is an example of user handler for the errors.
  * @param  error error-code.
  * @retval None
  * @note   If needed, implement it, and then attach it:
  *           + motor->AttachErrorHandler(&ErrorHandler);
  */
void ErrorHandler(uint16_t error)
{
    /* Printing to the console. */
    pc.printf("Error: %d.\r\n", error);
    
    /* Aborting the program. */
    exit(EXIT_FAILURE);
}


/* Main ----------------------------------------------------------------------*/

int main()
{
    
    pc.baud(115200);
    
    /*----- Initialization. -----*/

    /* Initializing SPI bus. */
    DevSPI dev_spi(D11, D12, D13);

    /* Initializing Motor Control Component. */
    motor = new L6474(D2, D8, D7, D9, D10, dev_spi);
    if (motor->init(&init) != COMPONENT_OK) {
        exit(EXIT_FAILURE);
    }

    /* Attaching and enabling the user handler for the flag interrupt. */
    motor->attach_flag_irq(&FlagIRQHandler);
    motor->enable_flag_irq();

    /* Printing to the console. */
    pc.printf("Motor Control Application Example for 1 Motor\r\n\n");
    
    
    while(1)
    {
        ReadCommand();
        InterpetCommand();
    }

}

void ReadCommand(void)
{
    szCommand[0] = 0;
    int pos =0;
    char c;

    do{
    c = pc.getc();
    //pc.putc(c);
    szCommand[pos] = c;
    pos++;
    }while(c!='\r');
    szCommand[pos]=0;
   
    //printf("\r\n");
   
    //printf("CMD : %s\r\n",szCommand);
}

void InterpetCommand(void)
{
   
    char * token = strtok(szCommand, " ");
    char CMD[100];
   
    //printf( "Token %s\n", token ); //printing each token
   
   
    if(token == NULL)
        strcpy(CMD,szCommand);    
    else
        strcpy(CMD,token);
       
    //printf( "CMD %s\n", CMD);
       
   
    if(strcmp(CMD,"MOVE")==0)
    {
        motor->enable();
        token = strtok(NULL, " ");
        int Position=0;
        sscanf(token,"%d",&Position);  
        targetPosition = Position;
        Move();
        printf("OK\n");
        motor->disable();
        return;
    }
       
       
    if(strncmp(CMD,"RESET",5)==0)
    {
        wheel.reset();    
        motor->set_home();  
        printf( "OK\n"); //printing each token
        return;
    }

    if(strncmp(CMD,"GET_MOT_POS",11)==0)
    {
        /* Getting current position */
        int position = motor->get_position();
        
        printf( "%d\n", position); //printing each token
        return;
    }
   
    if(strncmp(CMD,"GET_ENC_POS",11)==0)
    {
        printf( "%d\n",wheel.getPulses()); //printing each token
        return;
    }
    
   
    printf( "ERROR\n");
   
}
 
void Move()
{
    /* Requesting to go to a specified position. */
    motor->go_to(targetPosition);

    /* Waiting while the motor is active. */
    motor->wait_while_active();
       
    wait_us(20000);  
}

