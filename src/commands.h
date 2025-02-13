//
// Created by xabdomo on 2/13/25.
//

#ifndef COMMANDS_H
#define COMMANDS_H

// just copy an entire file as is
#define COPY_FILE ((char) 0x01)

// copy a specific block from a specific file
#define COPY_BLOCK ((char) 0x02)

// rewrite this entire block with the following x bytes
#define WRITE_BLOCK ((char) 0x03)

// this file is complete .. no more copying or blocks
#define DONE ((char) 0x04)

#endif //COMMANDS_H
