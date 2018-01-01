#include <hipe.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

hipe_session session;

hipe_loc getLoc(char* id) {
    hipe_send(session, HIPE_OP_GET_BY_ID, 0, 0, 1, id); 
    hipe_instruction instruction;
    hipe_instruction_init(&instruction);
    hipe_await_instruction(session, &instruction, HIPE_OP_LOCATION_RETURN);
    return instruction.location;
}

int main(int argc, char** argv) {


    if(argc < 2) {
        printf("Usage: %s (filename), where (filename) is a JPEG image.\n", argv[0]);
        return 1;
    }

    FILE* file = fopen(argv[1], "r");

    if(!file) {
        printf("Could not open file: '%s' for reading.\n", argv[1]);
        return 2;
    }

    /*Determine filesize by seeking to end first.*/
    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    printf("File size: %li bytes.\n", size);
    rewind(file);

    char* data = malloc(size);

    size_t result = fread(data, 1, size, file); //load file into character array.
    if(result != size) {
        printf("Error reading file\n");
        return 2;
    }

    session = hipe_open_session((argc>2) ? argv[2] : 0,0,0,argv[0]);
    if(!session) return 3; /*quit if can't connect to Hipe.*/

    hipe_send(session, HIPE_OP_APPEND_TAG, 0,0, 2, "img", "theimage");
    hipe_loc img = getLoc("theimage");

    hipe_instruction instr;
    hipe_instruction_init(&instr);
    instr.opcode = HIPE_OP_SET_SRC;
    instr.location = img;
    instr.arg[0] = data; instr.arg_length[0]=size;
    instr.arg[1] = "image/jpeg"; instr.arg_length[1]=strlen(instr.arg[1]);

    hipe_send_instruction(session, instr);

    while(1)
        sleep(1);
}
