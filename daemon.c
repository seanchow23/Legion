/*
 * Legion: Command-line interface
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h> // Include to define bool, true, and false

typedef enum {
    DAEMON_INACTIVE,
    DAEMON_ACTIVE,
    DAEMON_STARTING,
    DAEMON_STOPPING,
    DAEMON_EXITED,
    DAEMON_CRASHED,
    DAEMON_UNKNOWN
} DaemonState;


// helper functions
void register_daemon(char* name, char* cmd, FILE *out);
void unregister_daemon(char* name, FILE *out);
void daemon_status(char* name, FILE *out);
void status_all(FILE *out);
void start(char* name, FILE *out);
const char* daemon_state(DaemonState state);



typedef struct daemon {
    char* name;  // Name of the daemon
    char* command; // Command to run the daemon
    int pid;     // Process ID of the daemon
    int state;   // State of the daemon
    struct daemon *next; // Pointer to next daemon in the list
} daemon_t;

daemon_t *head = NULL; // Head of the linked list

int registered_daemon_count = 0; // Counter for the number of registered daemons


void run_cli(FILE *in, FILE *out) {
    // Main function to handle command line interface interaction
    char line[2048];  // Buffer to store the input line, large enough to handle typical user commands

    // Infinite loop to continuously prompt the user for commands
    while (1) {
        sf_prompt();  // Display the command line prompt, defined elsewhere, typically writes "legion>" to stdout
        fprintf(out, "legion> ");  // Print the prompt to the output file/stream
        fflush(out);  // Ensure the prompt is output immediately by flushing the stream

        // Read a line of input from the user
        if (fgets(line, sizeof(line), in) == NULL) {
            sf_error("Error reading the input");  // Call error handling function if input reading fails
            break;  // Exit the loop if there is an error in input (e.g., EOF or read error)
        }

        // Remove the newline character at the end of the input line
        line[strcspn(line, "\n")] = 0;

        // Tokenize the line using space as a delimiter to find the command
        char *cmd = strtok(line, " ");
        if (cmd == NULL) continue; // If no command was entered (empty line), start the loop again

        // Handling different commands
        if (strcmp(cmd, "help") == 0) {
            // If the command is "help", print the list of available commands
            fprintf(out, "Available commands:\n");
            fprintf(out, "help (0 args) Print this help message\n");
            fprintf(out, "quit (0 args) Quit the program\n");
            fprintf(out, "register (0 args) Register a daemon\n");
            fprintf(out, "unregister (1 args) Unregister a daemon\n");
            fprintf(out, "status (1 args) Show the status of a daemon\n");
            fprintf(out, "status-all (0 args) Show the status of all daemons\n");
            fprintf(out, "start (1 args) Start a daemon\n");
            fprintf(out, "stop (1 args) Stop a daemon\n");
            fprintf(out, "logrotate (1 args) Rotate log files for a daemon\n");
        } else if (strcmp(cmd, "quit") == 0) {
            // If the command is "quit", break the loop to terminate the CLI
            break;
        } else if (strcmp(cmd, "register") == 0) {
            // Handle the "register" command to add a new daemon
            char *name = strtok(NULL, " ");
            char *command = strtok(NULL, "\0"); // Take the rest of the line as the command
            if (!name || !command) {
                fprintf(out, "Error: Invalid arguments. Usage: register <name> <command>\n");
            } else {
                register_daemon(name, command, out);
            }
        } else if (strcmp(cmd, "unregister") == 0) {
            // Handle the "unregister" command to remove a daemon
            char *name = strtok(NULL, " ");
            if (!name) {
                fprintf(out, "Error: Missing daemon name. Usage: unregister <name>\n");
            } else {
                unregister_daemon(name, out);
            }
        } else if (strcmp(cmd, "status") == 0) {
            // Handle the "status" command to display the status of a specific daemon
            char *name = strtok(NULL, " ");
            if (!name) {
                fprintf(out, "Error: Missing daemon name. Usage: status <name>\n");
            } else {
                daemon_status(name, out);
            }
        } else if (strcmp(cmd, "status-all") == 0) {
            // Handle the "status-all" command to display the status of all daemons
            status_all(out);
        } else if (strcmp(cmd, "start") == 0) {
            // Handle the "start" command to start a daemon
            char *name = strtok(NULL, " ");
            if (!name) {
                fprintf(out, "Error: Missing daemon name. Usage: start <name>\n");
            } else {
                start(name, out);
            }
        } else {
            // If none of the known commands match output an error message
            fprintf(out, "Unknown command: %s\n", cmd);
        }
    }
}

// start to be implemented
void start(char* name, FILE* out) {
  
}

// Helper function to copy a string into a newly allocated space and handle errors.
bool allocate_and_copy(char** dest, const char* src) {
    *dest = strdup(src);  // Duplicate the source string and assign it to the destination pointer.
    if (!*dest) {
        sf_error("Memory allocation failed.");  // Log an error if memory allocation fails.
        return false;  // Return false to indicate failure.
    }
    return true;  // Return true to indicate success.
}

// Helper function to check if a daemon with a given name is already registered in the linked list.
bool is_daemon_registered(const char* name) {
    daemon_t *current = head;  // Start from the head of the linked list.
    while (current != NULL) {  // Traverse the entire list.
        if (strcmp(current->name, name) == 0)  // Check if the current daemon's name matches the given name.
            return true;  // Return true if a match is found, indicating the daemon is registered.
        current = current->next;  // Move to the next node in the list.
    }
    return false;  // Return false if no match is found, indicating the daemon is not registered.
}

// Function to register a new daemon.
void register_daemon(char* name, char* cmd, FILE *out) {
    if (is_daemon_registered(name)) {
        sf_error("Daemon already registered.");  // Log an error if the daemon is already registered.
        return;  // Exit the function to prevent re-registration.
    }

    daemon_t *new_daemon = malloc(sizeof(daemon_t));  // Allocate memory for a new daemon structure.
    if (!new_daemon) {
        sf_error("Memory allocation failed.");  // Log an error if memory allocation fails.
        return;  // Exit the function due to the failure.
    }

    // Attempt to allocate and copy the name and command into the new daemon structure.
    if (!allocate_and_copy(&new_daemon->name, name) || !allocate_and_copy(&new_daemon->command, cmd)) {
        free(new_daemon);  // Free the allocated memory for the daemon structure if either copy fails.
        return;  // Exit the function due to the failure.
    }

    // Initialize the new daemon's properties.
    new_daemon->pid = -1;  // Set the process ID to -1, indicating the daemon is not yet running.
    new_daemon->state = DAEMON_INACTIVE;  // Set the initial state to inactive.
    new_daemon->next = head;  // Insert the new daemon at the beginning of the linked list.
    head = new_daemon;  // Update the head of the list to point to the new daemon.

    sf_register(name, cmd);  // Register the new daemon with the system, assuming sf_register logs the registration.
}

// Function to unregister a daemon by its name.
void unregister_daemon(char* name, FILE *out) {
    daemon_t **indirect = &head;  // Pointer to pointer used to iterate through the linked list and modify it.
    // Traverse the list to find the daemon with the specified name.
    while (*indirect && strcmp((*indirect)->name, name) != 0) {
        indirect = &(*indirect)->next;  // Move to the next element.
    }
    // If no matching daemon is found log an error and exit the function.
    if (*indirect == NULL) {
        sf_error("Daemon with the specified name not found.");
        return;
    }
    // Check if the daemon is in an inactive state before allowing it to be unregistered.
    if ((*indirect)->state != DAEMON_INACTIVE) {
        sf_error("Daemon is not in the inactive state.");
        return;
    }

    // Remove the daemon from the list.
    daemon_t *to_delete = *indirect;  // Store the pointer to the daemon to be removed.
    *indirect = to_delete->next;  // Bypass the deleted daemon in the list.
    // Free the allocated memory for the daemon's properties.
    free(to_delete->name);
    free(to_delete->command);
    free(to_delete);  // Free the memory allocated for the daemon structure itself.
    sf_unregister(name);  // Perform any additional cleanup and logging associated with unregistering a daemon.
}

// Function to return a string representing a daemon's state based on the DaemonState enum.
const char* daemon_state(DaemonState state) {
    switch(state) {  // Switch on the state value to return the corresponding string.
        case DAEMON_INACTIVE: return "inactive";
        case DAEMON_ACTIVE: return "active";
        case DAEMON_STARTING: return "starting";
        case DAEMON_STOPPING: return "stopping";
        case DAEMON_EXITED: return "exited";
        case DAEMON_CRASHED: return "crashed";
        default: return "unknown";  // Handle any undefined or erroneous state values.
    }
}

// Function to print the status of a specific daemon.
void daemon_status(char* name, FILE* out) {
    // Iterate through the linked list of daemons.
    for (daemon_t *current = head; current != NULL; current = current->next) {
        // Check if the current daemon's name matches the requested name.
        if (strcmp(current->name, name) == 0) {
            // Print the daemon's name, process ID, and state to the output.
            fprintf(out, "%s\t%d\t%s\n", current->name, current->pid, daemon_state(current->state));
            return;  // Exit the function after printing the status.
        }
    }
    // If no daemon with the specified name is found, log an error.
    sf_error("Daemon with the specified name not found.");
}

// This function checks the status of all daemons registered in the linked list and outputs their status.
void status_all(FILE *out) {
    char status_message[256];  // Buffer to accumulate status messages for reporting to the testing framework.

    // Check if there are no daemons registered and report to the user if the list is empty.
    if (head == NULL) {
        fprintf(out, "No daemons registered.\n");  // Output a message indicating no daemons are registered.
        return;  // Exit the function since there are no daemons to process.
    }

    // Iterate over each daemon in the linked list.
    for (daemon_t *current = head; current != NULL; current = current->next) {
        // Output the name, process ID, and state of each daemon to the specified output file/stream.
        fprintf(out, "%s\t%d\t%s\n", current->name, current->pid, daemon_state(current->state));
    }

    // Use the sf_status function to log the status report to a testing framework or another monitoring tool.
    
    sf_status(status_message);

    // Additionally, print the status message buffer to the console for user visibility.
    fprintf(out, "%s\n", status_message);  // Output the status message to the console.
}