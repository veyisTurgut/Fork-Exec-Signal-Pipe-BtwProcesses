#include <iostream> //cout, endl include <csignal> include <fstream>
#include <fstream>  // outputfile <<
#include <csignal>
#include <unistd.h> //sleep

/**
 * \file process.cpp
 * \author Adalet Veyis Turgut 2017400210
 * \brief Codes of the "process". It does not do much. Writes to file that it was born, waits for signals and writes the signal id. 
 * \date 2020-12-26
 *  
 */
/// To write the output of processes
std::ofstream output_file;
///id of process, not pid. ex: 15 in P15, 3 in P3. Starts from 0.
int id;

/**
 * \brief Handles all signals except sigterm. All it does is writing the message to \ref output_file. 
 * \param i Represents the number of signals. Ex: 1 for sighup, 4 for sigill, etc.
 */
void handler(int i)
{
    output_file << "P" << id << " received signal " << i << std::endl;
}

/**
 * \brief Handles sigterm signal. Writes to \ref output_file and terminates the process.
 * \param i Represents the number of sigterm signal. 15 in this case.
 */
void sigtermHandler(int i)
{
    output_file << "P" << id << " received signal " << i << ", terminating gracefully" << std::endl;
    exit(i);
}

/**
 * \brief Main method of the process.
 * 
 * 
 * When \ref watchdog exec()s the process, process starts from here as expected. It expects 2 arguments namely: \ref id and \ref output_file. 
 * It also maps the signals with corresponding handler methods. Writes to \ref output_file that it was born, then sleeps until receiving a signal.
 * If it cant locate the \ref output_file, then it terminates with error.
 * 
 * \param argc the length of the command line arguments
 * \param argv array of the command line arguments
 *
 */
int main(int argc, char *argv[])
{
    ///open the file in append mode so that processes continue rather than overwriting.
    output_file.open(argv[2], std::ios::app);
    if (!output_file)
    {
        std::cout << "Unable to open file " << argv[2] << std::endl;
        exit(1); // terminates with error
    }
    id = std::stoi(argv[1]);
    ///Reports that it was born
    output_file << "P" << id << " is waiting for a signal" << std::endl;
    ///Maps signals with their handler fucntions ( \ref handler and \ref sigtermHandler ). 
    std::signal(SIGHUP, handler);
    std::signal(SIGINT, handler);
    std::signal(SIGILL, handler);
    std::signal(SIGTRAP, handler);
    std::signal(SIGFPE, handler);
    std::signal(SIGBUS, handler);
    std::signal(SIGSEGV, handler);
    std::signal(SIGTERM, sigtermHandler);
    std::signal(SIGXCPU, handler);
    ///An infinite loop of sleep(1) function, it waits until receiving a signal then sleeps again.
    while (1)
        sleep(1);
    return 0;
}
