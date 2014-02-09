#include <string>
#include <cstdlib>

#include <fcntl.h>
#include <unistd.h>

/** Length of the buffer for reading user input */
#define BUFFER_LENGTH 1024
/**
 * Type of line delimiter from the Arduino serial monitor; This should match
 * your selection in the serial monitor window
 */
#define LINE_DELIMETER '\n'

// Keep track of the current working directory
// When the application starts, the user is dropped into the root directory
std::string g_workingDir = "/";

/**
 * Read the user's input and delete any leading whitespace
 */
std::string readInput (const std::string currentDir);

/**
 * Execute the user's command in the current directory
 */
int execute (const std::string currentDir, std::string cmd);

/**
 * Change the working directory to whatever the user input
 */
void cd (std::string cmd);

void setup () {
    // Fool the Galileo into thinking that /dev/ttyGS0 (that's equivalent to the
    // serial monitor in the Arduino IDE) is stdout and stderr. This means that
    // output from all commands will be displayed on the serial monitor instead
    // of through the 3.5mm serial port
    int properFile = open("/dev/ttyGS0", O_WRONLY);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    dup2(properFile, STDOUT_FILENO);
    dup2(properFile, STDERR_FILENO);
    close(properFile);

    // Still going to use the Serial class for reading user input because I
    // haven't gotten stdin to work correctly yet
    Serial.begin(115200);
}

void loop () {
    std::string cmd;
    bool commandWasCD = false;

    // Get the user's input
    cmd = readInput(g_workingDir);

    // Echo the command
    printf("%s%c", cmd.c_str(), LINE_DELIMETER);
    fflush(stdout);

    // Make a note of whether or not the command was cd
    if (2 <= cmd.length())
        commandWasCD = 0 == cmd.compare(0, 2, "cd");

    // Execute the command and check for an error
    if (0 == execute(g_workingDir, cmd))
        // If no errors and the command was CD, make the change permanent
        if (commandWasCD)
            cd(cmd);
}

std::string readInput (const std::string currentDir) {
    uint16_t numChars = 0;
    char buffer[BUFFER_LENGTH];
    std::string *cmd;

    // Give the user a command prompt
    printf("root@galileo:%s$ ", currentDir.c_str());
    fflush(stdout);

    // Read characters from the terminal until a cmd is given...
    while (!numChars)
        numChars = Serial.readBytesUntil(LINE_DELIMETER, buffer,
        BUFFER_LENGTH - 1);

    // and append a null-terminator to the buffer... because apparently
    // readBytesUntil() doesn't do that
    buffer[numChars] = '\0';

    // Remove leading whitespace
    cmd = new std::string(buffer);
    while (' ' == (*cmd)[0])
        cmd->erase(0, 1);

    return buffer;
}

int execute (const std::string currentDir, std::string cmd) {
    std::string command = "cd " + currentDir + " && " + cmd;
    return system(command.c_str());
}

void cd (std::string cmd) {
    // Remove the "cd"
    cmd.erase(0, 2);

    // Remove any extra whitespace
    while (' ' == cmd[0])
        cmd.erase(0, 1);

    // If the user entered no arguments, let's default that to the home
    // directory, "/home/root"
    if (0 == cmd.length())
        g_workingDir = "~";
    else {
        // If the new directory starts from the root, overwrite the previous
        if ('/' == cmd[0])
            g_workingDir = cmd;
        // Otherwise, concatenate the two paths
        else {
            if (0 != g_workingDir.compare("/"))
                g_workingDir += '/';
            g_workingDir += cmd;

            // Remove a trailing '/' if it exists
            if ('/' == g_workingDir[g_workingDir.length() - 1])
                g_workingDir.erase(g_workingDir.length() - 1, 1);
        }
    }

    if (0 == g_workingDir.compare("/home/root"))
        g_workingDir = "~";
}
