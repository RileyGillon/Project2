#include <iostream>
#include <vector>
#include <string>
#include <queue>
#include <algorithm>
#include <iomanip>
#include <tuple>
using namespace std;

// Structure to represent a process control block
struct PCB {
    string id;
    int priority;
    int burst_time;
    int arrival_time;
    int remaining_time;
    int last_run_time;
    int remaining_quantum;
    int completion_time;
    int waiting_time;
    int turnaround_time;
    int queue_order; // To track FIFO order for round robin
};

// Custom comparator for priority queue (max heap based on priority, then FIFO for same priority)
struct ComparePriority {
    bool operator()(const PCB* a, const PCB* b) {
        // Higher priority value means higher priority (should come first)
        if (a->priority != b->priority) {
            return a->priority < b->priority; // max heap
        }
        // If same priority, use queue_order for FIFO (lower order = came first = should go first)
        return a->queue_order > b->queue_order;
    }
};

int main() {
    string line;
    char dummy;
    int time_quantum;
    vector<PCB> processes;

    // Read time quantum from standard input
    cin >> dummy >> time_quantum;

    // Read processes from standard input
    while (cin >> line) {
        PCB p;
        p.id = line;
        cin >> p.priority >> p.burst_time >> p.arrival_time;
        p.remaining_time = p.burst_time;
        p.last_run_time = -1;
        p.remaining_quantum = time_quantum;
        p.completion_time = 0;
        p.waiting_time = 0;
        p.turnaround_time = 0;
        p.queue_order = 0; // Will be set when added to ready queue
        processes.push_back(p);
    }

    // CPU Scheduler Simulator
    // We use a priority queue (ready queue) to manage processes ready to execute
    // Processes are ordered by priority (higher priority first), then by FIFO for same priority
    
    priority_queue<PCB*, vector<PCB*>, ComparePriority> ready_queue;
    vector<tuple<int, int, string, int>> schedule; // (start_time, end_time, process_id, priority)
    
    int current_time = 0;
    int completed = 0;
    int total_processes = processes.size();
    PCB* current_process = nullptr;
    int process_index = 0;
    int queue_counter = 0; // To track order of processes entering ready queue
    
    // Sort processes by arrival time for easier processing
    sort(processes.begin(), processes.end(), [](const PCB& a, const PCB& b) {
        return a.arrival_time < b.arrival_time;
    });
    
    int total_cpu_time = 0;
    
    while (completed < total_processes) {
        // Add all processes that have arrived by current_time to ready queue
        while (process_index < total_processes && processes[process_index].arrival_time <= current_time) {
            processes[process_index].queue_order = queue_counter++;
            ready_queue.push(&processes[process_index]);
            process_index++;
        }
        
        // If no process is ready and we have processes left, jump to next arrival time (Idle)
        if (ready_queue.empty() && process_index < total_processes) {
            int idle_start = current_time;
            current_time = processes[process_index].arrival_time;
            schedule.push_back(make_tuple(idle_start, current_time, "Idle", 0));
            continue;
        }
        
        // If ready queue is empty and no more processes, we're done
        if (ready_queue.empty()) {
            break;
        }
        
        // Get the highest priority process from ready queue
        current_process = ready_queue.top();
        ready_queue.pop();
        
        // Determine how long this process will run
        int run_time = min(current_process->remaining_quantum, current_process->remaining_time);
        
        // Check if a higher priority process arrives during this run
        int preempt_time = -1;
        for (int t = current_time + 1; t < current_time + run_time; t++) {
            // Check if any process arrives at time t
            int idx = process_index;
            while (idx < total_processes && processes[idx].arrival_time == t) {
                if (processes[idx].priority > current_process->priority) {
                    // Higher priority process arrived, preempt current process
                    preempt_time = t;
                    break;
                }
                idx++;
            }
            if (preempt_time != -1) break;
        }
        
        if (preempt_time != -1) {
            run_time = preempt_time - current_time;
        }
        
        // Execute the process
        int start_time = current_time;
        current_time += run_time;
        current_process->remaining_time -= run_time;
        current_process->remaining_quantum -= run_time;
        current_process->last_run_time = start_time;
        
        schedule.push_back(make_tuple(start_time, current_time, current_process->id, current_process->priority));
        total_cpu_time += run_time;
        
        // Add newly arrived processes to ready queue
        while (process_index < total_processes && processes[process_index].arrival_time <= current_time) {
            processes[process_index].queue_order = queue_counter++;
            ready_queue.push(&processes[process_index]);
            process_index++;
        }
        
        // Check if process is complete
        if (current_process->remaining_time == 0) {
            current_process->completion_time = current_time;
            completed++;
        } else {
            // Process is not complete
            bool was_preempted = (preempt_time != -1);
            
            if (was_preempted) {
                // Preempted by higher priority - reset quantum for fairness
                current_process->remaining_quantum = time_quantum;
            } else if (current_process->remaining_quantum == 0) {
                // Time quantum expired naturally, reset it
                current_process->remaining_quantum = time_quantum;
            }
            // else: process used partial quantum and will return with remaining quantum intact
            
            // All processes that re-enter queue get new queue_order (go to back for round robin)
            current_process->queue_order = queue_counter++;
            ready_queue.push(current_process);
        }
    }
    
    // Output the schedule - merge consecutive time slots for same process
    if (!schedule.empty()) {
        int merge_start = get<0>(schedule[0]);
        int merge_end = get<1>(schedule[0]);
        string merge_id = get<2>(schedule[0]);
        int merge_prio = get<3>(schedule[0]);
        
        for (size_t i = 1; i < schedule.size(); i++) {
            int start = get<0>(schedule[i]);
            int end = get<1>(schedule[i]);
            string id = get<2>(schedule[i]);
            int prio = get<3>(schedule[i]);
            
            // If same process and same priority, merge
            if (id == merge_id && prio == merge_prio) {
                merge_end = end;
            } else {
                // Output the merged segment
                if (merge_id == "Idle") {
                    cout << "Time " << merge_start << "-" << merge_end << ": " << merge_id << endl;
                } else {
                    cout << "Time " << merge_start << "-" << merge_end << ": " << merge_id << " (Priority " << merge_prio << ")" << endl;
                }
                // Start new segment
                merge_start = start;
                merge_end = end;
                merge_id = id;
                merge_prio = prio;
            }
        }
        // Output the last segment
        if (merge_id == "Idle") {
            cout << "Time " << merge_start << "-" << merge_end << ": " << merge_id << endl;
        } else {
            cout << "Time " << merge_start << "-" << merge_end << ": " << merge_id << " (Priority " << merge_prio << ")" << endl;
        }
    }
    
    // Calculate and output turnaround and waiting times
    cout << "\nTurnaround Time" << endl;
    for (auto& p : processes) {
        p.turnaround_time = p.completion_time - p.arrival_time;
        cout << p.id << " = " << p.turnaround_time << endl;
    }
    
    cout << "\nWaiting Time" << endl;
    for (auto& p : processes) {
        p.waiting_time = p.turnaround_time - p.burst_time;
        cout << p.id << " = " << p.waiting_time << endl;
    }
    
    cout << "\nCPU Utilization Time" << endl;
    cout << total_cpu_time << "/" << current_time << endl;
    
    return 0;
}
