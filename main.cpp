#include "process.h"
#include "ioModule.h"
#include "processMgmt.h"

#include <chrono> // for sleep
#include <thread> // for sleep


int main(int argc, char* argv[])
{
    // single thread processor
    // it's either processing something or it's not
    bool processorAvailable = true;

    // vector of processes, processes will appear here when they are created by
    // the ProcessMgmt object (in other words, automatically at the appropriate time)
    list<Process> processList;
    
    // this will orchestrate process creation in our system, it will add processes to 
    // processList when they are created and ready to be run/managed
    ProcessManagement processMgmt(processList);

    // this is where interrupts will appear when the ioModule detects that an IO operation is complete
    list<IOInterrupt> interrupts;   

    // this manages io operations and will raise interrupts to signal io completion
    IOModule ioModule(interrupts);  

    // Do not touch
    long time = 1;
    long sleepDuration = 50;
    string file;
    stringstream ss;
    enum stepActionEnum {noAct, admitNewProc, handleInterrupt, beginRun, continueRun, ioRequest, complete} stepAction;

    // Do not touch
    switch(argc)
    {
        case 1:
            file = "./procList.txt";  // default input file
            break;
        case 2:
            file = argv[1];         // file given from command line
            break;
        case 3:
            file = argv[1];         // file given
            ss.str(argv[2]);        // sleep duration given
            ss >> sleepDuration;
            break;
        default:
            cerr << "incorrect number of command line arguments" << endl;
            cout << "usage: " << argv[0] << " [file] [sleepDuration]" << endl;
            return 1;
            break;
    }

    processMgmt.readProcessFile(file);


    time = 0;
    processorAvailable = true;

    //keep running the loop until all processes have been added and have run to completion
    while(processMgmt.moreProcessesComing() || !processList.empty())
    {
        //Update our current time step
        ++time;

        //let new processes in if there are any
        processMgmt.activateProcesses(time);

        //update the status for any active IO requests
        ioModule.ioProcessing(time);

        //init the stepAction, update below
        stepAction = noAct;

        //   <your code here> 
        // conditional for handling interrupts
        if (!interrupts.empty())
        {
          IOInterrupt interrupt = interrupts.front();
          interrupts.pop_front();
          for (auto& process : processList)
          {
            if (process.id == interrupt.procID)
            {
              process.state = ready;
              stepAction = handleInterrupt;
              break;
            }
          }
        }

      // loop for admitting new processes
      for (auto& process : processList)
      {
        if (process.state == newArrival)
        {
          process.state = ready;
          stepAction = admitNewProc;
          break;
        }
      }

      //conditional for running processes
      if (processorAvailable)
      {
        for (auto& process : processList)
        {
          if (process.state == ready)
          {
            process.state = processing;
            processorAvailable = false;
            stepAction = beginRun;
            break;
          }
        }
      }
      else 
      {
        for (auto& process : processList)
        {
          if (process.state == processing)
          {
            process.processorTime++;
            if (!process.ioEvents.empty() && (process.processorTime >= process.reqProcessorTime))
            {
              ioModule.submitIORequest(time, process.ioEvents.front(), process);
              process.ioEvents.pop_front();
              process.state = blocked;
              processorAvailable = true;
              stepAction = ioRequest;
            }
            else if (process.processorTime >= process.reqProcessorTime)
            {
              process.state = done;
              process.doneTime = time;
              processorAvailable = true;
              stepAction = complete;
            }
            else
            {
              stepAction = continueRun;
            }

            break;
          }
        }
      }

        // Leave the below alone (at least for final submission, we are counting on the output being in expected format)
        cout << setw(5) << time << "\t"; 
        
        switch(stepAction)
        {
            case admitNewProc:
              cout << "[  admit]\t";
              break;
            case handleInterrupt:
              cout << "[ inrtpt]\t";
              break;
            case beginRun:
              cout << "[  begin]\t";
              break;
            case continueRun:
              cout << "[contRun]\t";
              break;
            case ioRequest:
              cout << "[  ioReq]\t";
              break;
            case complete:
              cout << "[ finish]\t";
              break;
            case noAct:
              cout << "[*noAct*]\t";
              break;
        }

        // You may wish to use a second vector of processes (you don't need to, but you can)
        printProcessStates(processList); // change processList to another vector of processes if desired
        
        this_thread::sleep_for(chrono::milliseconds(sleepDuration));
    }

    return 0;
}
