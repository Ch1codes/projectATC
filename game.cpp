#include <iostream>
#include <string>
#include <cstdlib>
#include <ctime>
#include <chrono>
#include <thread>
#include <vector>
#include <queue>
#include <mutex> 
#include <sstream>
#include <atomic>
#include <algorithm>

using namespace std;

const int BASE_PLANE_GEN_INTERVAL = 5;

string toUpper(const string &s) {
    string result = s;
    transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
}

class Plane {
public:
    string callSign;
    int fuel;
    int timeSinceRequest;
    int timeToLand;
    int priority;
    int landingDuration;
    bool permissionToLand;

    Plane(string cs, int f)
        : callSign(cs), fuel(f), timeSinceRequest(0),
          timeToLand(0), priority(0), landingDuration(0), permissionToLand(false)
    {
        landingDuration = 3 + (rand() % 8); //landingDuration set randombly between 3 to 10 seconds.
    }
};

struct Node {
    Plane* plane;
    Node* next;
    Node* prev;
    Node(Plane* p) : plane(p), next(nullptr), prev(nullptr) { }
};

class DoublyLinkedList {
public:
    Node* head;
    Node* tail;
    int size;

    DoublyLinkedList() : head(nullptr), tail(nullptr), size(0) { }

    void push_back(Plane* plane) {
        Node* newNode = new Node(plane);
        if (!head) {
            head = tail = newNode;
        } else {
            tail->next = newNode;
            newNode->prev = tail;
            tail = newNode;
        }
        size++;
    }

    void insertSorted(Plane* plane) {
        Node* newNode = new Node(plane);
        if (!head) {
            head = tail = newNode;
            size++;
            return;
        }
        Node* curr = head;
        while (curr && curr->plane->priority <= plane->priority)
            curr = curr->next;
        if (!curr) {
            tail->next = newNode;
            newNode->prev = tail;
            tail = newNode;
        } else if (curr == head) {
            newNode->next = head;
            head->prev = newNode;
            head = newNode;
        } else {
            newNode->prev = curr->prev;
            newNode->next = curr;
            curr->prev->next = newNode;
            curr->prev = newNode;
        }
        size++;
    }

    Node* find(const string& cs) {
        Node* curr = head;
        while (curr) {
            if (curr->plane->callSign == cs)
                return curr;
            curr = curr->next;
        }
        return nullptr;
    }

    void remove(Node* node) {
        if (!node) return;
        if (node == head) {
            head = node->next;
            if (head)
                head->prev = nullptr;
            else
                tail = nullptr;
        } else if (node == tail) {
            tail = node->prev;
            if (tail)
                tail->next = nullptr;
            else
                head = nullptr;
        } else {
            node->prev->next = node->next;
            node->next->prev = node->prev;
        }
        delete node;
        size--;
    }

    void clear() {
        Node* curr = head;
        while (curr) {
            Node* temp = curr;
            curr = curr->next;
            delete temp;
        }
        head = tail = nullptr;
        size = 0;
    }
};

string generateCallSign() {
    string cs;
    for (int i = 0; i < 4; i++) {
        cs.push_back('A' + rand() % 26);
    }
    return cs;
} //random 4 letters for callsign of the planes

int lastFuel = -1;
Plane* generatePlane() {
    int fuel;
    do {
        fuel = 10 + rand() % 51;
    } while (fuel == lastFuel);
    lastFuel = fuel;
    return new Plane(generateCallSign(), fuel);
}

void resortLandingQueue(DoublyLinkedList &landingQueue) {
    vector<Plane*> planes;
    Node* curr = landingQueue.head;
    while (curr) {
        planes.push_back(curr->plane);
        curr = curr->next;
    }
    sort(planes.begin(), planes.end(), [](Plane* a, Plane* b) {
        return a->priority < b->priority;
    }); //lamba comparator has been used here to arrange plane in ascending order of priority
    landingQueue.clear();
    for (Plane* p : planes)
        landingQueue.push_back(p);
} //landing queue is reordered based on the priority of the planes in it. 

void updateLandingQueuePriorities(DoublyLinkedList &landingQueue) {
    int newPriority = 0;
    Node* curr = landingQueue.head;
    while (curr) {
        curr->plane->priority = newPriority;
        newPriority++;
        curr = curr->next;
    }
}

void display(const DoublyLinkedList &airList, const DoublyLinkedList &landingQueue,
             const vector<Plane*> &landedList, int simulationSeconds, int score) {
    cout << "\033[2J\033[1;1H"; //ansi escape code used to clear the screen
    cout << "==== AIR TRAFFIC CONTROLLER GAME ====\n";
    cout << "Time elapsed: " << simulationSeconds << " seconds    Score: " << score << "\n";
    cout << "-------------------------------------\n";
    cout << "Airspace (Planes Waiting):\n";
    if (airList.size == 0) {
        cout << "  [None]\n";
    } else {
        Node* curr = airList.head;
        while (curr) {
            Plane* p = curr->plane;
            cout << "  CallSign: " << p->callSign
                 << " | Fuel: " << p->fuel << "%"
                 << " | Wait: " << p->timeSinceRequest << "s"
                 << " | Landing Duration: " << p->landingDuration << "s"
                 << "\n";
            curr = curr->next;
        }
    }
    cout << "-------------------------------------\n";
    cout << "Landing Queue (sorted by effective priority ascending):\n";
    if (landingQueue.size == 0) {
        cout << "  [None]\n";
    } else {
        Node* curr = landingQueue.head;
        while (curr) {
            Plane* p = curr->plane;
            cout << "  CallSign: " << p->callSign
                 << " | Fuel: " << p->fuel << "%"
                 << " | TimeToLand: " << p->timeToLand << "s"
                 << " | Effective Priority: " << p->priority << "\n";
            curr = curr->next;
        }
    }
    cout << "-------------------------------------\n";
    cout << "Landed Planes:\n";
    if (landedList.empty()) {
        cout << "  [None]\n";
    } else {
        for (size_t i = 0; i < landedList.size(); i++) {
            cout << "  " << i + 1 << ". " << landedList[i]->callSign << "\n";
        }
    }
    cout << "-------------------------------------\n";
    cout << "Commands:\n";
    cout << "  To order landing: <CALLSIGN> <PRIORITY>   (e.g., abcd 0)\n";
    cout << "  To delay landing: delay <CALLSIGN>          (returns plane to airspace)\n";
    cout << "-------------------------------------\n";
    cout.flush();
}

class Simulation {
private:
    DoublyLinkedList airList;
    DoublyLinkedList landingQueue;
    vector<Plane*> landedList;
    int simulationSeconds;
    int planeGenTimer;
    bool gameOver;
    int score;
    int dynamicPlaneGenInterval;
    queue<string> commandQueue; //for storing the commands of user in a thread safe queue
    mutex commandMutex;
    atomic<bool> running; //for controlling the running state of input thread
    thread userInputThread; //for taking user input asynchronously

    void inputThreadFunc() {
        while (running.load()) {
            string input;
            if (!getline(cin, input))
                break;
            if (!input.empty()) {
                lock_guard<mutex> lock(commandMutex);
                commandQueue.push(input);
            }
        }
    } //this runs while simulation is running, reading a line from the user and exiting if the input fails, the user input is read continuously in a separate thread and pushed to a command queue

public:
    Simulation()
        : simulationSeconds(0), planeGenTimer(0), gameOver(false),
          score(0), dynamicPlaneGenInterval(BASE_PLANE_GEN_INTERVAL), running(true)
    {
        userInputThread = thread(&Simulation::inputThreadFunc, this); //user input thread started bound to inputThreadFunc
    }

    ~Simulation() {
        running.store(false); 
        if (userInputThread.joinable())
            userInputThread.join();
    }//safely stop the input thread

    void updateAirspace() {
        Node* curr = airList.head;
        while (curr) {
            Plane* p = curr->plane;
            p->timeSinceRequest++;
            p->fuel -= 1 + (score / 100);
            if (p->fuel <= 0) {
                cout << "\n*** Plane " << p->callSign << " ran out of fuel! ***\nGame Over!\n";
                gameOver = true;
                return;
            }
            curr = curr->next;
        }
        if (airList.size > 10) {
            cout << "\n*** Overcrowding: More than 10 planes in the air! ***\nGame Over!\n";
            gameOver = true;
        }
    }

    void processLandingQueue() {
        Node* curr = landingQueue.head;
        bool nonFrontZero = false;
        while (curr) {
            curr->plane->timeToLand--;
            if (curr != landingQueue.head && curr->plane->timeToLand <= 0) {
                nonFrontZero = true;
                break;
            }
            curr = curr->next;
        }
        if (nonFrontZero) {
            cout << "\n*** Game Over: A plane not at the front reached zero time to land! ***\n";
            gameOver = true;
            return;
        }
        if (landingQueue.head && landingQueue.head->plane->timeToLand <= 0) {
            Node* landingNode = landingQueue.head;
            Plane* landedPlane = landingNode->plane;
            cout << "\n+++ Plane " << landedPlane->callSign << " has landed successfully! +++\n";
            int landingScore = 10 + landedPlane->fuel;
            score += landingScore;
            cout << "     Score +" << landingScore << " (Total: " << score << ")\n";
            landingQueue.head = landingNode->next;
            if (landingQueue.head)
                landingQueue.head->prev = nullptr;
            else
                landingQueue.tail = nullptr;
            landingQueue.size--;
            delete landingNode;
            landedList.push_back(landedPlane);
            updateLandingQueuePriorities(landingQueue);
        }
    }

    void generatePlanes() {
        if (planeGenTimer >= dynamicPlaneGenInterval && airList.size < 10) {
            int numPlanes = 1;
            if ((rand() % 100) < 20 && airList.size <= 8)
                numPlanes = 2;
            for (int i = 0; i < numPlanes && (airList.size < 10); i++) {
                Plane* newPlane = generatePlane();
                airList.push_back(newPlane);
            }
            if (numPlanes > 1)
                planeGenTimer = -dynamicPlaneGenInterval;
            else
                planeGenTimer = 0;
        }
    }

    void processUserCommands() {
        lock_guard<mutex> lock(commandMutex);
        while (!commandQueue.empty()) {
            string command = commandQueue.front();
            commandQueue.pop();
            istringstream iss(command);
            string token;
            iss >> token;
            token = toUpper(token);
            if (token == "DELAY") {
                string callSign;
                if (!(iss >> callSign)) {
                    cout << ">> Invalid delay command. Usage: delay <CALLSIGN>\n";
                    continue;
                }
                callSign = toUpper(callSign);
                Node* target = landingQueue.find(callSign);
                if (target) {
                    Plane* p = target->plane;
                    p->permissionToLand = false;
                    if (target == landingQueue.head) {
                        landingQueue.head = target->next;
                        if (landingQueue.head)
                            landingQueue.head->prev = nullptr;
                        else
                            landingQueue.tail = nullptr;
                    } else if (target == landingQueue.tail) {
                        landingQueue.tail = target->prev;
                        if (landingQueue.tail)
                            landingQueue.tail->next = nullptr;
                        else
                            landingQueue.head = nullptr;
                    } else {
                        target->prev->next = target->next;
                        target->next->prev = target->prev;
                    }
                    landingQueue.size--;
                    delete target;
                    airList.push_back(p);
                    cout << ">> Order: Landing for plane " << p->callSign 
                         << " has been delayed and returned to airspace.\n";
                } else {
                    cout << ">> Order Error: No plane with call sign " << callSign << " found in landing queue.\n";
                }
            } else {
                string callSign = token;
                int userPriority;
                if (!(iss >> userPriority)) {
                    cout << ">> Invalid command format. Use: <CALLSIGN> <PRIORITY>\n";
                    continue;
                }
                callSign = toUpper(callSign);
                Node* target = airList.find(callSign);
                if (target) {
                    Plane* p = target->plane;
                    p->permissionToLand = true;
                    for (Node* node = landingQueue.head; node != nullptr; node = node->next) {
                        if (node->plane->priority >= userPriority)
                            node->plane->priority++;
                    }
                    p->priority = userPriority;
                    if (target == airList.head) {
                        airList.head = target->next;
                        if (airList.head)
                            airList.head->prev = nullptr;
                        else
                            airList.tail = nullptr;
                    } else if (target == airList.tail) {
                        airList.tail = target->prev;
                        if (airList.tail)
                            airList.tail->next = nullptr;
                        else
                            airList.head = nullptr;
                    } else {
                        target->prev->next = target->next;
                        target->next->prev = target->prev;
                    }
                    airList.size--;
                    delete target;
                    p->timeToLand = p->landingDuration;
                    landingQueue.insertSorted(p);
                    resortLandingQueue(landingQueue);
                    updateLandingQueuePriorities(landingQueue);
                    cout << ">> Order: Plane " << p->callSign 
                         << " granted landing permission with effective priority " << p->priority 
                         << ". Estimated time to land: " << p->timeToLand << " seconds.\n";
                } else {
                    cout << ">> Order Error: No plane with call sign " << callSign << " found in airspace.\n";
                }
            }
        }
    }// the user command from command queue is read and differentiated and action is taken based on it. uses <sstream> header for processing the string input

    void displayState() {
        display(airList, landingQueue, landedList, simulationSeconds, score);
    }

    void run() {
        cout << "Welcome to the Air Traffic Controller Simulation.\n\n"
             << "Your goal is to safely manage and land incoming planes.\n\n"
             << "GAME OVER CONDITIONS:\n"
             << "  1. A plane runs out of fuel (fuel <= 0).\n"
             << "  2. Overcrowding: More than 10 planes in the airspace.\n"
             << "  3. Multiple planes landing simultaneously.\n"
             << "  4. If a plane that is not at the front of the landing queue reaches zero time to land.\n\n"
             << "Score points by landing planes safely. Avoid collisions and fuel runout.\n"
             << "Good luck!\n\n";
        cout << "Press ENTER twice to start...";
        cin.ignore();

        while (!gameOver) {
            simulationSeconds++;
            planeGenTimer++;
            dynamicPlaneGenInterval = max(3, BASE_PLANE_GEN_INTERVAL - (score / 50));
            updateAirspace();
            if (gameOver)
                break;
            processLandingQueue();
            generatePlanes();
            processUserCommands();
            displayState();
            this_thread::sleep_for(chrono::seconds(3));
        }
        cout << "\n=== GAME OVER ===\n";
        cout << "Total planes landed: " << landedList.size() << "\n";
        cout << "Final Score: " << score << "\n";
    }
};

int main() {
    srand(static_cast<unsigned int>(time(NULL))); //the random number generator is seeded with current time. 
    Simulation sim;
    sim.run();
    return 0;
}
