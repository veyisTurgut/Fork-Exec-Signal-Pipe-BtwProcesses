/**
 * \mainpage Cmpe 322 Project 1 - Watchdog
 * \author Adalet Veyis Turgut 2017400210
 * \brief Code compiles and runs smoothly. Bonus case is not handled. Ignore warnings after compilation. 
 * 
 * Since I used sleep(1) function excessively, it runs a bit slow. Actually my computer runs smoothly with sleep(0.4), 
 * but your computer is gonna evaluate and it may be slow compared to mine. To eliminate possible conflicts, I kept the time interval high.
 * 
 * To compile: g++ -o watchdog ./watchdog.cpp.
 * 
 * To run (executor must be running!): ./watchdog \ref number_of_processes \ref process_output_path \ref watchdog_output_path.
 * 
 * \date 2020-12-26
 * 
 * 
 */
/**
 * \file watchdog.cpp
 * \author Adalet Veyis Turgut 2017400210
 * \brief Codes of the Watchdog process. It monitors the system and restarts dead processes.
 * 
 * Detailed explanation is given in main() method. 
 * 
 * \date 2020-12-26
 */

#include <string>  //std::string
#include <fstream> //file open close
#include <csignal> //kill-signal
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h> //strlen
#include <iostream> //std::cout
#include <stdlib.h>

/// File that watchdog writes
std::ofstream watchdog_output_file;
/// Path of the file that processes will write their messages, watchdog just passes to each \ref process.cpp while exec()ing.
std::string process_output_path;
/// Path of the file that watcHdog will write its messages.
std::string watchdog_output_path;
/// Total number of all processes. It will be given as a command line argument.
int number_of_processes;
/// Location of the pipe.
char *myfifo = "/tmp/myfifo";
/// Names of the pipe. We will communicate with executor.cpp using this pipe.
int fd;

/**
 * \brief This function forks a \ref process.cpp and execs it with given index parameter, finally sends necessary message to executor using pipe ( \ref fd).
 * 
 * First, it forks a child process. Then, updates the corresponding value of pid array and sends "Px pidxx" message to executor with pipe in parent part.
 * On the other hand, child part writes to the watchdog output file and exec()s the process with given parameter  index. 
 * 
 * \param index Id of the process.
 * \param array_of_process_ids Array of pids of all processes.
 */
void execOneChild(int index, int *array_of_process_ids)
{
    ///Try to fork
    pid_t childpid = fork();
    if (childpid < 0)
    /// Fork failed, exit.
    {
        std::cout << "fork failed\n"
                  << std::endl;
    }
    else if (childpid == 0)
    /// We are in child, write to file ( \ref watchdog_output_file) that process is started and exec().
    {
        watchdog_output_file << "P" << index << " is started and it has a pid of " << (long)getpid() << std::endl;
        char *arguments[] = {"./process", &std::to_string(index)[0], &process_output_path[0], NULL};
        /// Here I used execv(). It takes arguments as an array. As usual first argument(argv[0]) is the process itself.
        /// Second and third are id and \ref process_output_path. Finish the array with NULL.
        /// Since it is a C function, there are no strings. Changing string to char array is cumbersome.
        /* std::cout << "execing" <<*/ execv("./process", arguments);
    }
    else
    /// We are in parent, update the \ref array_of_process_ids and write pid of the child process to pipe. Add sleep(1) for synchronization.
    {
        array_of_process_ids[index] = childpid;
        std::string message = "P" + std::to_string(index) + " " + std::to_string(array_of_process_ids[index]);
        sleep(1);
        write(fd, message.c_str(), strlen(message.c_str()) + 1);
    }
}

/**
 * \brief Calls \ref execOneChild() function \ref number_of_processes times.
 * 
 * \param array_of_process_ids Array of pids of all processes
 */
void execChildren(int *array_of_process_ids)
{
    for (int i = 1; i <= number_of_processes; i++)
    {
        execOneChild(i, array_of_process_ids);
    }
}

/**
 * \brief Find and return the index of the killed process in array, -1 if not found.
 * 
 * \param value Pid of the killed process
 * \param array_of_process_ids Array of pids of all processes
 * \return Integer index of the killed process
 */
int findIndex(int value, int *array_of_process_ids)
{
    for (int i = 2; i <= number_of_processes; i++)
    {
        if (value == array_of_process_ids[i])
        {
            return i;
        }
    }
    return -1;
}

/**
 * \brief Sigterm signal handler
 * When everything ends, executor sends a signal to watchdog and watchdog terminates gracefully.
 * \param i Equals to 15. Value of sigterm signal.
 */
void sigtermHandler(int i)
{
    watchdog_output_file << "Watchdog is terminating gracefully" << std::endl;
    exit(i);
}

/**
 * \brief Main method of the project. 
 * 
 * We start with reading command line arguments and initializing global variables. If there is something wrong with arguments, we simply quit. 
 * Then open the pipe and send the watchdog pid to executor. Initiate all the processes by fork() and exec(). Send their pids to executor and wait-sleep
 * until one the children dies. If dead child is P1 then kill all children and restart one by one, else just restart the dead process. If executor sends
 * a sigterm signal, terminate gracefully.
 * 
 * \param argc count of command line arguments
 * \param argv array of command line arguments 
 */
int main(int argc, char *argv[])
{

    if (argc != 4)
    {
        fprintf(stderr, "Not enough arguments");
        exit(1); // terminate with error
    }
    ///Initialize global variables with given command line arguments
    number_of_processes = std::stoi(argv[1]);
    int array_of_process_ids[number_of_processes + 1]; // arr[i] = Pi, dont want to mess up with arr[0]
    process_output_path = argv[2];
    watchdog_output_path = argv[3];

    //to clear the possible contents of output file. Since processes open with append mode, had to do it here.
    std::ofstream process_output_file;
    process_output_file.open(process_output_path);
    process_output_file.close();

    ///Open the watchdog file with given path \ref watchdog_output_path
    watchdog_output_file.open(watchdog_output_path);
    if (!watchdog_output_file)
    {
        std::cout << "Unable to open file " << watchdog_output_path << std::endl;
        exit(1); // terminate with error
    }

    ///Open the pipe \ref fd
    fd = open(myfifo, O_WRONLY);
    ///Send watchdog's pid to executor: "P0 pidxxx"
    std::string watchdog_message = "P0 ";
    watchdog_message += std::to_string(getpid());
    write(fd, &watchdog_message[0], watchdog_message.size() + 1);

    ///Initiate the processes
    execChildren(array_of_process_ids);

    ///Map sigterm signal with its handler fucntion \ref sigtermHandler
    std::signal(SIGTERM, sigtermHandler);

    while (1)
    {
        //for debug purposes
        /* for (int i = 1; i <= number_of_processes; i++)
        {
            std::cout << array_of_process_ids[i] << " ";
        } */
        //std::cout << std::endl;
        //std::cout << "waiting for my child to die" << std::endl;
        ///Wait, do nothing, sleep. Until one of the processes dies.
        pid_t killed_process_pid = wait(NULL);
        // std::cout << "killed_process_pid " << killed_process_pid << std::endl;

        if (killed_process_pid == array_of_process_ids[1])
        /// P1 is killed, kill others and restart all.
        {
            //kill all other processes
            watchdog_output_file << "P1 is killed, all processes must be killed" << std::endl;
            for (int i = 2; i <= number_of_processes; i++)
            {
                kill(array_of_process_ids[i], SIGTERM);
                sleep(1);
            }
            //restart all processes
            watchdog_output_file << "Restarting all processes" << std::endl;
            execChildren(array_of_process_ids);
        }
        else if (killed_process_pid > array_of_process_ids[1])
        ///One of the other processes is killed, restart it.
        // the fact that I used that condition in if is that when P1 dead all processes are recreated one by one,
        // in addition each new process receives and increasing pid. Therefore we can safely assume this condition
        // is satisfactory.
        {
            //find the index of process
            int index = findIndex(killed_process_pid, array_of_process_ids);

            //now fork and exec it
            watchdog_output_file << "P" << index << " is killed" << std::endl;
            watchdog_output_file << "Restarting P" << index << std::endl;
            execOneChild(index, array_of_process_ids);
        }
    }
    // Probably we wont reach here. Executor will terminate us :(.
    close(fd);
    return 0;
}