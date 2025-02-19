typedef struct {
    char name[20]; 
    int message_id;
    float mult_factor;
    float add_factor; 
    int offset; 
    int length; 
} Message_operator; 

const int NUM_MESSAGE_OPERATORS = 1;
const Message_operator message_operators[] = {
    // define your operators here
    {"RPM", 0x0A5, 0.25, 0, 5, 2},
};