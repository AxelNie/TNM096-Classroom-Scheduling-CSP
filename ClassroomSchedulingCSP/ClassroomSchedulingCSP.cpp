#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <random>
#include <ctime>
#include <algorithm>
#include <chrono>

using namespace std;


class Course {
public:
    string code;
    
    Course() : code("") {}
    
    Course(const string& code) : code(code) {}

    bool has_same_first_digit(const Course& other) const {
        return code[2] == other.code[2];
    }

	bool operator==(const Course& other) const {
		return code == other.code;
	}
};

class CSP {
public:
    map<string, vector<Course>> classrooms;
    vector<string> time_slots;
    vector<Course> courses;

    CSP(const vector<Course>& courses) : courses(courses) {
        time_slots = { "9 am", "10 am", "11 am", "12 pm", "1 pm", "2 pm", "3 pm", "4 pm" };
    }

    bool conflict(const std::map<std::string, std::vector<Course>>& current_schedule, int time_slot_idx, const std::string& classroom) {
        const Course& value = current_schedule.at(classroom).at(time_slot_idx);

        for (const auto& room : current_schedule) {
            if (room.first != classroom) { // Check other classrooms only
                if (time_slot_idx < room.second.size()) { // Ensure time_slot_idx is within bounds
                    const Course& course = room.second[time_slot_idx]; // Get the course at the same time_slot_idx
                    if (course.code != value.code && course.has_same_first_digit(value) &&
                        !(course.code == "MT501" && value.code == "MT502") &&
                        !(course.code == "MT502" && value.code == "MT501")) {
                        return true;
                    }
                }
            }
        }
        return false;
    }


    int total_conflicts(const map<string, vector<Course>>& current) const {
        int total_conflicts = 0;

        // Iterate through each time slot
        for (size_t i = 0; i < time_slots.size(); ++i) {
            // Iterate through each classroom
            for (const auto& classroom1 : current) {
                if (i < classroom1.second.size()) {
                    const Course& course1 = classroom1.second[i];

                    // Compare with courses in other classrooms at the same time slot
                    for (const auto& classroom2 : current) {
                        if (classroom1.first != classroom2.first && i < classroom2.second.size()) {
                            const Course& course2 = classroom2.second[i];

                            // Check if the courses have the same first digit, and are not the MT501 and MT502 pair
                            bool same_first_digit = course1.has_same_first_digit(course2);
                            bool mt501_mt502_pair = (course1.code == "MT501" && course2.code == "MT502") || (course1.code == "MT502" && course2.code == "MT501");

                            if (same_first_digit && !mt501_mt502_pair) {
                                total_conflicts++;
                            }
                        }
                    }
                }
            }
        }

        return total_conflicts;
    }

};

void display_schedule(const map<string, vector<Course>>& schedule);

tuple<string, int, Course> find_random_conflict(const map<string, vector<Course>>& current_schedule, CSP& csp);

std::map<std::string, std::vector<Course>> create_random_assignment(CSP& csp);

map<string, vector<Course>> min_conflicts(CSP& csp, int max_steps);

void display_schedule(const map<string, vector<Course>>& schedule);

vector<Course> create_courses(const vector<string>& course_codes);

int count_unsatisfied_preferences(const map<string, vector<Course>>& schedule, const CSP& csp);

map<string, vector<Course>> min_conflicts_with_preferences(CSP& csp, int max_steps, int max_attempts);


int main() {
    vector<string> course_codes = { "MT101", "MT102", "MT103", "MT104", "MT105", "MT106", "MT107", "MT201",
                                             "MT202", "MT203", "MT204", "MT205", "MT206", "MT301", "MT302", "MT303",
                                             "MT304", "MT401", "MT402", "MT403", "MT501", "MT502" };
    vector<Course> courses = create_courses(course_codes);
    CSP csp(courses);
    //map<string, vector<Course>> schedule = min_conflicts(csp, 100000);
    map<string, vector<Course>> schedule = min_conflicts_with_preferences(csp, 100000, 1000);
    
    if (schedule.empty()) {
        cout << "No solution found within the maximum number of steps." << endl;
    }
    else {
        cout << "Final solution" << endl;
        display_schedule(schedule);
    }

    return 0;
}

map<string, vector<Course>> min_conflicts_with_preferences(CSP& csp, int max_steps, int max_attempts) {
    int min_unsatisfied_preferences = numeric_limits<int>::max();
    map<string, vector<Course>> best_schedule;

    for (int attempt = 0; attempt < max_attempts; ++attempt) {
        map<string, vector<Course>> current_schedule = min_conflicts(csp, max_steps);

        if (!current_schedule.empty()) {
            int unsatisfied_preferences = count_unsatisfied_preferences(current_schedule, csp);

            if (unsatisfied_preferences < min_unsatisfied_preferences) {
                min_unsatisfied_preferences = unsatisfied_preferences;
                best_schedule = current_schedule;
            }
        }
    }

    return best_schedule;
}


int count_unsatisfied_preferences(const map<string, vector<Course>>& schedule, const CSP& csp) {
    int unsatisfied_preferences = 0;
    const auto& time_slots = csp.time_slots;

    for (const auto& classroom : schedule) {
        for (size_t i = 0; i < classroom.second.size(); ++i) {
            const Course& course = classroom.second[i];
            const string& time_slot = time_slots[i];

            if (time_slot == "9 am" || time_slot == "12 pm" || time_slot == "4 pm") {
                unsatisfied_preferences++;
            }

            if ((course.code == "MT501" || course.code == "MT502") && (time_slot != "1 pm" && time_slot != "2 pm")) {
                unsatisfied_preferences++;
            }
        }
    }

    return unsatisfied_preferences;
}

map<string, vector<Course>> min_conflicts(CSP& csp, int max_steps) {


    // Create a random complete assignment
    map<string, vector<Course>> current_schedule = create_random_assignment(csp);

    for (int i = 0; i < max_steps; i++) {
        if (csp.total_conflicts(current_schedule) == 0) {
            return current_schedule;
        }

        tuple<string, int, Course> out = find_random_conflict(current_schedule, csp);
        string conflicted_classroom = get<0>(out);
        int conflicted_time_slot = get<1>(out);


        // Iterate through all classrooms and time slots
        int min_conflict_count = numeric_limits<int>::max();
        string best_classroom;
        int best_swap_idx = -1;
        for (auto& classroom : current_schedule) {
            for (size_t i = 0; i < classroom.second.size(); ++i) {
                // Swap courses
                swap(current_schedule[conflicted_classroom][conflicted_time_slot], classroom.second[i]);

                // Calculate conflicts after swap
                int conflict_count = csp.total_conflicts(current_schedule);

                // Check if the swap reduces conflicts
                if (conflict_count < min_conflict_count) {
                    min_conflict_count = conflict_count;
                    best_classroom = classroom.first;
                    best_swap_idx = i;
                }

                // Swap courses back
                swap(current_schedule[conflicted_classroom][conflicted_time_slot], classroom.second[i]);
            }
        }

        // Swap with the best course
        if (best_swap_idx != -1) {
            swap(current_schedule[conflicted_classroom][conflicted_time_slot], current_schedule[best_classroom][best_swap_idx]);
        }
    }



    // Return an empty assignment if no solution is found within max_steps
    return map<string, vector<Course>>();
}

tuple<string, int, Course> find_random_conflict(const map<string, vector<Course>>& current_schedule, CSP& csp) {
    // Find all conflicting time slots and classrooms
    vector<tuple<string, int, Course>> conflicts;
    for (const auto& classroom : current_schedule) {
        for (size_t time_slot_idx = 0; time_slot_idx < classroom.second.size(); ++time_slot_idx) {
            if (csp.conflict(current_schedule, time_slot_idx, classroom.first)) {
                conflicts.emplace_back(classroom.first, time_slot_idx, classroom.second[time_slot_idx]);
            }
        }
    }

    if (conflicts.empty()) {
        throw runtime_error("No conflicting time slots and classrooms found.");
    }

    // Pick a random conflicting time slot and classroom
    // Get the current time as a seed
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::mt19937 mt(seed);

    uniform_int_distribution<int> conflict_dist(0, conflicts.size() - 1);
    int random_conflict_idx = conflict_dist(mt);
    return conflicts[random_conflict_idx];
}


vector<Course> create_courses(const vector<string>& course_codes) {
    vector<Course> courses;

    for (const auto& code : course_codes) {
        courses.emplace_back(code);
    }
    return courses;
}

void display_schedule(const map<string, vector<Course>>& schedule) {
    string time_slots[] = {"9 am", "10 am", "11 am", "12 pm", "1 pm", "2 pm", "3 pm", "4 pm"};
    cout << "K3---- SP34---- TP51" << endl;
    for (size_t i = 0; i < 8; ++i) {
        cout << time_slots[i] << " ";
        for (const auto& room : schedule) {
            if (i < room.second.size()) {
                cout << room.second[i].code << " ";
            }
            else {
                cout << "----- ";
            }
        }
        cout << endl;
    }
}

std::map<std::string, std::vector<Course>> create_random_assignment(CSP& csp) {
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_int_distribution<int> time_dist(0, csp.time_slots.size() - 1);
    std::uniform_int_distribution<int> course_dist(0, csp.courses.size() - 1);

    std::map<std::string, std::vector<Course>> current_schedule;

    // Create a random complete assignment
    std::vector<Course> shuffled_courses = csp.courses;
    std::shuffle(shuffled_courses.begin(), shuffled_courses.end(), mt);

    for (const auto& course : shuffled_courses) {
        bool assigned = false;
        while (!assigned) {
            std::uniform_int_distribution<int> room_dist(0, 2);
            int room_idx = room_dist(mt);
            std::string room;

            switch (room_idx) {
            case 0:
                room = "TP51";
                break;
            case 1:
                room = "SP34";
                break;
            default:
                room = "K3";
                break;
            }

            if (current_schedule[room].size() < 8) {
                current_schedule[room].push_back(course);
                assigned = true;
            }
        }
    }

    // Print the initial random schedule
    std::cout << "Initial Random Schedule:" << std::endl;
    display_schedule(current_schedule);
    

    return current_schedule;
}


