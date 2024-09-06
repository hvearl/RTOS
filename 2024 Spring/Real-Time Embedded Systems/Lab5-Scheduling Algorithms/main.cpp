/*==================================================================================================
	To run the code use:
		g++ main.cpp
		./a.exe <input>.txt <output>.txt

	* NOTE: You have to add .txt to the file names
	
===================================================================================================
For RMA:
	The code currently ends the periodic task if it misses the deadline
	The code will take note of a missed Aperiodic deadline but continue to schedule and run the task

For EDF:


For LLF:


========================================================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <vector>
#include <thread>
#include <chrono>

using namespace std;
//void RMA_Prioritize(vector<Periodic>& tasks);


//periodic class
class Periodic {
public:
	int next_deadline;
	

	char task_ID;
	int exec_time;
	int period;

	//contructor
	Periodic(char task_ID,int e, int p) :task_ID(task_ID),exec_time(e), period(p),next_deadline(p) {}
	//destructor
	//~Periodic() {};
	friend void RMA_Prioritize(vector<Periodic>& tasks);
 };
//aperiodic class
class Aperiodic {
public:
	int task_deadline;
	bool Complete;
	bool Miss;
	

	char task_ID;
	int exec_time;
	int release_time;

	//contructor
	Aperiodic(char task_ID,int e, int p) :task_ID(task_ID),exec_time(e), release_time(p),task_deadline(p + 500),Complete(false),Miss(false) {}

	//destructor
	friend void Sort_Atasks(vector<Aperiodic>& A_tasks);
	
};

void RMA_Prioritize(vector<Periodic>& tasks) {
	sort(tasks.begin(), tasks.end(), [](const Periodic& a, const Periodic& b) {
		return a.period < b.period;
		});
}

void Sort_Atasks(vector<Aperiodic>& A_tasks) {
	sort(A_tasks.begin(), A_tasks.end(), [](const Aperiodic& a, const Aperiodic& b) {
		return a.release_time < b.release_time;
		});
}

void RMA_RUN(vector<Periodic>& tasks, vector<Aperiodic>& A_tasks, int sim_time, int num_tasks, int num_async) {
	int elapsed_time = 0;
	int current_time=0;
    int current_task_index = 0;

	// aperiodic ---
	int A_task_index = 0;

	int sum_wcet = 0;
	for(int k = 0; k < num_tasks; k++){
		sum_wcet += tasks[k].exec_time;
	}
	//-----

	cout << "Running task " << tasks[current_task_index].task_ID << " at time " << elapsed_time << endl;
	for(;elapsed_time < sim_time; elapsed_time++,current_time++) {
        // Run the current task for its execution time
		
		// aperiodic 

		for(int m = 0; m < num_async; m++){ // check that the task has not expired
			if(elapsed_time>A_tasks[m].task_deadline && !A_tasks[m].Complete && !A_tasks[m].Miss){ // add and not complete
				cout<<"Task "<<A_tasks[m].task_ID<< " missed its deadline at time "<<elapsed_time<<endl;
				A_tasks[m].Miss = true;
			}
		}

			//check if deadline missed
			//print error message
			//move on to next task
		if(elapsed_time>tasks[current_task_index].next_deadline){
			cout<<"Task "<<tasks[current_task_index].task_ID<< " missed its deadline at time "<<elapsed_time<<endl;
			//move on to next task
			tasks[current_task_index].next_deadline+=tasks[current_task_index].period;
			current_time=0;
			current_task_index = (current_task_index + 1) % tasks.size();
			cout << "Running task " << tasks[current_task_index].task_ID << " at time " << elapsed_time << endl;
		}
        // Move to the next task (wrap around if necessary)
		if(current_time==tasks[current_task_index].exec_time){
			tasks[current_task_index].next_deadline+=tasks[current_task_index].period;
			current_time=0;
			current_task_index = (current_task_index + 1) % tasks.size();
			cout << "Running task " << tasks[current_task_index].task_ID << " at time " << elapsed_time<< endl ; //" next deadline "<< tasks[current_task_index].next_deadline << endl;


			// aperiodic tasks 
			for (int j = 0; j < num_async; j++) { // for each aperiodic task
				if(elapsed_time>A_tasks[A_task_index].release_time && current_task_index + 1 == num_tasks && A_tasks[A_task_index].Complete == false){
					if((elapsed_time + tasks[current_task_index].exec_time + A_tasks[A_task_index].exec_time) < tasks[0].next_deadline ){ // the task has enough slack to run
						bool not_valid = false;
						for(int k = 0; k < num_tasks; k++){
							if ((elapsed_time + sum_wcet + A_tasks[A_task_index].exec_time) > tasks[k].next_deadline){
								not_valid = true;
							}
						}
						if(!not_valid && elapsed_time + tasks[current_task_index].exec_time < sim_time){
							cout << "Running in slack task " << A_tasks[A_task_index].task_ID << " at time " << elapsed_time + tasks[current_task_index].exec_time << endl;
							A_tasks[A_task_index].Complete = true;
							elapsed_time += A_tasks[A_task_index].exec_time;
						}else{
							not_valid = false;
						}
					}
				}
				A_task_index++;
			}
			A_task_index = 0;
		}


	
	}
	cout<<"Simulation ended at time "<<elapsed_time<<endl;
}
/*void EDF_Prioritize(vector<Periodic>& tasks){
	
}

void EDF_RUN(vector<Periodic>& tasks, int exec_time){
	
}
*/
int main(int argc, char *argv[]){
	if(argc!=3){
		printf("Error: Invalid number of command-line arguments. %d given. Please try again.", argc);
		exit(1);
	}
	
	ifstream infile(argv[1]);
	if(!infile.is_open()){
		cout<<"Error opening input file."<<endl;
		exit(1);
	}
	
	ofstream outfile(argv[2]);
	if(!outfile.is_open()){
		cout<<"Error opening output file."<<endl;
		exit(1);
	}
	
	int num_tasks;
	string line;
	getline(infile, line);
	istringstream iss(line);
	iss>>num_tasks;
	cout<<"Number of tasks:"<< num_tasks<< endl;
	
	int exec_time;
	getline(infile,line);
	istringstream iss2(line);
	iss2>>exec_time;
	cout<<"Execution time:"<<exec_time<<endl;
	
	//get info from files
	char id;
	char comma;
	int exec_temp, period_temp;
	vector<Periodic> tasks;
	for(int i=0; i<num_tasks;i++){
		infile>>id>>comma>>exec_temp>>comma>>period_temp;
		tasks.emplace_back(id, exec_temp, period_temp);
	}
	RMA_Prioritize(tasks); 
	//fetch aperiodic tasks
	for (const auto& task:tasks) {
		cout <<task.task_ID<< " Execution Time: " << task.exec_time << " " << "Period: " << task.period << endl;
	}
	int num_async;
	infile>>num_async;
	//create table
	auto tms1 =new int[num_async][2];
	auto ids1 =new char[num_async];

	vector<Aperiodic> A_tasks;
		for(int i=0; i<num_async;i++){
			infile>>id>>comma>>exec_temp>>comma>>period_temp;
			ids1[i]=id;
			tms1[i][0]=exec_temp;
			tms1[i][1]=period_temp;

			A_tasks.emplace_back(id, exec_temp, period_temp);
		}
	infile.close();

	cout << "Number of asyncronous tasks:" << num_async << endl;
	cout<<"File contains the following:"<<endl;
	/*for (int i = 0; i < num_tasks; i++) {
		cout<<ids[i]<<" ";
        for (int j = 0; j < 2; j++) {
            cout << tms[i][j] << " ";
        }
        cout << endl;
    }
	*/

	// sort A_tasks
	Sort_Atasks(A_tasks); 
	//fetch aperiodic tasks
	for (const auto& task:A_tasks) {
		cout <<task.task_ID<< " Execution Time: " << task.exec_time << " " << "Release Time: " << task.release_time << endl;
	}
	/*
	// print the aperiodic task to screen
	for (int i = 0; i < num_async; i++) {
		cout<<ids1[i]<<" ";
        for (int j = 0; j < 2; j++) {
            cout << tms1[i][j] << " ";
        }
        cout << endl;
    }*/

	
	/*
	for (int i = 0; i < num_tasks; i++) {
		outfile<<ids[i]<<" ";
        for (int j = 0; j < 2; j++) {
            outfile << tms[i][j] << " ";
        }
        outfile << endl;
    }*/
	for (int i = 0; i < num_async; i++) {
		outfile<<ids1[i]<<" ";
        for (int j = 0; j < 2; j++) {
            outfile << tms1[i][j] << " ";
        }
        outfile << endl;
    }
 //prioritize tasks based on RMA (fixed)
	RMA_RUN(tasks, A_tasks, exec_time, num_tasks, num_async);  //run RMA simulation
	
	/*EDF_Prioritize(tasks);
	EDF_RUN(tasks);
	
	LLF_Prioritize(tasks);
	EDF_RUN(tasks);
	*/

	outfile.close();
	
	
}

