/* 
This program simulates a simple linux shell. It loops infinitely, prompting the user for commands. Regular commands and piping/redirection
are handled with the system() call, while the "&" feature is manually implemented to allow for commands to run in the background. Special
piping uses the $ symbol and aggregates the output of the left-side commands to be the input of each right-side command.
*/

#include<iostream>
#include<cstring>
#include<vector>
#include<string>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/wait.h>

#define READ 0
#define WRITE 1

using namespace std;

/* rightSideCommand()
Executes the $ pipe operation on a single command on the right side given the input commands. For example, on an 
input ls pwd $ wc, this function will first execute ls and pwd and writing the output to a shared pipe, then execute wc 
by reading the contents of the pipe.
Parameters:
    -inCmds is a vector of the input (left-side) commands. In the above example, it would contain {"ls", "pwd"}
    -outCmd is a string representing the right-side command to execute, such as "wc"
*/


int rightSideCommand(vector<string> inCmds, string outCmd) {

    // Create a pipe
    int fds[2];
    int p = pipe(fds);
    if (p == -1) {
        printf("Error creating a pipe for a right-side command\n");
        exit(0);
    }

    // Execute left-side commands

    int leftChild = fork();

    if (leftChild == -1) {
        printf("Error forking for left-side commands\n");
        exit(0);
    }
    // Child code
    else if (leftChild == 0) {

        // Set output to pipe
        dup2(fds[WRITE], fileno(stdout));

        // Unneeded
        close(fds[READ]);
        close(fds[WRITE]);

        // Execute each left-side command
        for (int i = 0; i < inCmds.size(); i++) {
            //leftSideCommand(inCmds.at(i), fds);

            int leftCmd = fork();

            if (leftCmd == -1) {
                exit(0);
            } else if (leftCmd == 0) {

                char* command[] = {inCmds.at(i).data(), NULL};
                execvp(inCmds.at(i).c_str(), command);
                printf("Error in execvp for a left-side command\n");
                exit(0);

            } else {
                int status;
                waitpid(leftCmd, &status, 0);
            }
        }
        exit(0);
    }
    // Parent code
    else {
        int status;

        waitpid(leftChild, &status, 0);
    }

    

    // Execute right-side command

    dup2(fds[READ], fileno(stdin));

    // Unneeded
    close(fds[READ]);
    close(fds[WRITE]);

    int child = fork();

    if (child == -1) {
        printf("Error forking for a right-side command\n");
        exit(0);
    }
    // Child code
    else if (child == 0) {

        char* rightCommand[] = {outCmd.data(), NULL};
        execvp(outCmd.c_str(), rightCommand);
        printf("Error in execvp for a right-side command\n");
        exit(0);
    }
    // Parent code
    else {
        int status;
        waitpid(child, &status, 0);
    }

    return 0;
}


/* executeNormalCommand()
Executes all non-$-pipe commands using system(), and checks the string for the background "&" feature, waiting for
the child to finish executing if it is not there.
Parameter:
    -cmd is a string representing the command to execute
*/

int executeNormalCommand(string cmd) {

    int background = cmd.find("&");

    pid_t child = fork();
    // Child code
    if (child == 0) {
        
        if (background != string::npos) {
            // Remove "&", since it's handled manually
            cmd = cmd.substr(0,strlen(cmd.c_str())-2);
        }

        system(cmd.c_str());
        exit(0);

    } else if (child < 0) {
        printf("Error forking a child\n");
        exit(0);
    }
    // Parent code
    else {
        // Wait for child if no "&"
        if (background == string::npos) {
            int status;
            waitpid(child, &status, 0);
        } else {
            return 0;
        }
    }
    return 0;
}


/* main()
Infinite while loop prompting the user for commands and parsing the command for $. Passes commands to other functions above for 
execution, and performs some input validation.
*/

int main()
{

    cout << "\nType \"exit\" to quit the program\n\n" << endl;

    while(true) {
        string prompt = "/User/ % ";
        cout << prompt;
        string cmd;
        getline(cin, cmd);

        // Check for exit command
        if (strcmp(cmd.c_str(), "exit") == 0) {
            return 0;
        }

        int dollar = cmd.find("$");

        // No special piping, handles everything other than $ commands
        if (dollar == string::npos) {
            // Some input validation for invalid commands
            int ch1 = cmd.find("<");
            int ch2 = cmd.find(">");
            int ch3 = cmd.find("|");
            if (ch1 == 0 || ch1 == strlen(cmd.c_str())-1 || ch2 == 0 || ch2 == strlen(cmd.c_str())-1 || ch3 == 0 || ch3 == strlen(cmd.c_str())-1) {
                printf("Invalid command. Please try again\n");
            } else {
                ch1 = cmd.rfind("<");
                ch2 = cmd.rfind(">");
                ch3 = cmd.rfind("|");
                if (ch1 == 0 || ch1 == strlen(cmd.c_str())-1 || ch2 == 0 || ch2 == strlen(cmd.c_str())-1 || ch3 == 0 || ch3 == strlen(cmd.c_str())-1) {
                    printf("Invalid command. Please try again\n");
                } else {
                    executeNormalCommand(cmd);
                }
            }       
        }

        // Special piping, used for $ inputs
        else {

            // Parse the string
            string left = cmd.substr(0, dollar);
            int end = strlen(cmd.c_str())-dollar;
            string right = cmd.substr(dollar+1, end);

            // parse left side input
            int space = left.rfind(" ");
            while (space == strlen(left.c_str())-1) {
                // Trim whitespace from end of left side
                left = left.substr(0, strlen(left.c_str())-1);
                space = left.rfind(" ");
            }

            space = left.find(" ");
            while (space == 0) {
                // Trim whitespace from beginning of left side
                left = left.substr(1, strlen(left.c_str())-1);
                space = left.find(" ");
            }

            // parse right-side command(s)
            space = right.rfind(" ");
            while (space == strlen(right.c_str())-1) {
                // Trim whitespace from end of right side
                right = right.substr(0, strlen(right.c_str())-1);
                space = right.rfind(" ");
            }

            space = right.find(" ");
            while (space == 0) {
                // Trim whitespace from beginning of right side
                right = right.substr(1, strlen(right.c_str())-1);
                space = right.find(" ");
            }


            vector<string> inCmds;
            space = left.find(" ");
            if (space != string::npos) {
                // Two left-side commands
                inCmds.push_back(left.substr(0, space));
                inCmds.push_back(left.substr(space+1, strlen(left.c_str())-space));
            } else {
                // Only one left-side command
                inCmds.push_back(left);
            }
        
            vector<string> outCmds;
            space = right.find(" ");
            if (space != string::npos) {
                // Two left-side commands
                outCmds.push_back(right.substr(0, space));
                outCmds.push_back(right.substr(space+1, strlen(right.c_str())-space));
            } else {
                // Only one left-side command
                outCmds.push_back(right);
            }

            // Pass left and right side commands to execution function, for each right-side command

            for (int i = 0; i < outCmds.size(); i++) {

                int rightCmd = fork();
                if (rightCmd == -1) {
                    printf("Error forking for a right-side command\n");
                } else if (rightCmd == 0) {
                    rightSideCommand(inCmds, outCmds.at(i));
                    exit(0);
                } else {
                    int status;
                    waitpid(rightCmd, &status, 0);
                }
            } 
        }
    }
    return 0; 
}