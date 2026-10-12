/*  HOSTEL MANAGEMENT SYSTEM  -  DSA Project (C)
 *
 *  Feature                    Data structure used
 *  -----------------------    ------------------------------------
 *  Student records            Array of structures
 *  Search student             Linear search, Binary search, Hashing
 *  Room allotment + undo      Stack
 *  Room waiting list          Circular queue
 *  Complaints                 Priority queue (sorted linked list)
 *  Gate entry / exit          Doubly linked list + singly linked list
 *  Hostel fee payments        AVL tree
 *  Fire exit route            Graph (BFS and Dijkstra)
 *  Sort students              Insertion, Selection, Merge sort
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_STUDENTS  100
#define NUM_ROOMS     12      /* 3 floors x 4 rooms: 101-104, 201-204, 301-304 */
#define ROOM_CAPACITY 4
#define HASH_SIZE     31
#define HOSTEL_FEE    100000
#define LAUNDRY_FEE   500
#define CURFEW_HOUR   22      /* 10 PM */


/* ======================= STUDENTS AND ROOMS ======================= */

typedef struct {
    int  roll;
    char name[50];
    char phone[15];
    char parentPhone[15];
    int  roomNo;        /* 0 = no room yet */
    int  laundryPaid;   /* 1 = paid for this month */
} Student;

typedef struct {
    int roomNo;
    int floor;
    int occupied;
} Room;

Student students[MAX_STUDENTS];
int studentCount = 0;

Room rooms[NUM_ROOMS];

/* Current date and time as text, e.g. "12-10 09:30 PM" */
void getTime(char *buf)
{
    time_t now = time(NULL);
    strftime(buf, 20, "%d-%m %I:%M %p", localtime(&now));
}

void freeMemory(void);          /* defined at the end, frees all linked nodes */

/* Throw away the rest of the current input line */
void clearLine(void)
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF)
        ;
}

/* Stop the program cleanly when input ends (Ctrl+D / end of file) */
void inputEnded(void)
{
    printf("\nInput ended.\n");
    freeMemory();
    exit(0);
}

/* Read one line of text (spaces allowed) into buf */
void readText(char *buf, int size)
{
    if (fgets(buf, size, stdin) == NULL)
        inputEnded();

    int len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n')
        buf[len - 1] = '\0';            /* remove the Enter key */
    else
        clearLine();                     /* line was too long: drop the rest */
}

/* Read a whole number. If letters are typed, ask again. */
int readInt(void)
{
    int x;
    while (scanf("%d", &x) != 1) {
        if (feof(stdin))
            inputEnded();
        clearLine();
        printf("Please enter a number: ");
    }
    clearLine();
    return x;
}

/* Read y or n, returns 1 for yes and 0 for no */
int readYesNo(void)
{
    char ans[10];
    readText(ans, sizeof ans);
    return ans[0] == 'y' || ans[0] == 'Y';
}


/* ======================= HASHING (chaining) ======================= */

typedef struct HashNode {
    int roll;
    int index;                  /* position in students[] */
    struct HashNode *next;
} HashNode;

HashNode *hashTable[HASH_SIZE];

int hashFunction(int roll)
{
    return roll % HASH_SIZE;
}

void hashInsert(int roll, int index)
{
    HashNode *node = malloc(sizeof(HashNode));
    if (node == NULL) {
        printf("Out of memory.\n");
        return;
    }
    int b = hashFunction(roll);

    node->roll = roll;
    node->index = index;
    node->next = hashTable[b];      /* insert at head of the chain */
    hashTable[b] = node;
}

int hashSearch(int roll)
{
    if (roll <= 0)                       /* negative roll would give a negative index */
        return -1;

    HashNode *p = hashTable[hashFunction(roll)];

    while (p != NULL) {
        if (p->roll == roll)
            return p->index;
        p = p->next;
    }
    return -1;
}


/* ======================= LINEAR AND BINARY SEARCH ======================= */

/* sortedIdx[] keeps student positions in increasing roll order,
   so binary search works even if students register in any order. */
int sortedIdx[MAX_STUDENTS];

int linearSearch(int roll)                  /* O(n) */
{
    for (int i = 0; i < studentCount; i++)
        if (students[i].roll == roll)
            return i;
    return -1;
}

int binarySearch(int roll)                  /* O(log n) */
{
    int low = 0, high = studentCount - 1;

    while (low <= high) {
        int mid = (low + high) / 2;
        int midRoll = students[sortedIdx[mid]].roll;

        if (midRoll == roll)
            return sortedIdx[mid];
        else if (midRoll < roll)
            low = mid + 1;
        else
            high = mid - 1;
    }
    return -1;
}


/* ======================= AVL TREE - FEE PAYMENTS ======================= */

typedef struct AVLNode {
    int roll;
    int height;
    struct AVLNode *left, *right;
} AVLNode;

AVLNode *feeRoot = NULL;
int feeCount = 0;

int height(AVLNode *n)  { return n == NULL ? 0 : n->height; }
int max(int a, int b)   { return a > b ? a : b; }

AVLNode *newAVLNode(int roll)
{
    AVLNode *n = malloc(sizeof(AVLNode));
    if (n == NULL) {
        printf("Out of memory.\n");
        return NULL;
    }
    n->roll = roll;
    n->height = 1;
    n->left = n->right = NULL;
    return n;
}

AVLNode *rightRotate(AVLNode *y)
{
    AVLNode *x = y->left;
    y->left = x->right;
    x->right = y;
    y->height = 1 + max(height(y->left), height(y->right));
    x->height = 1 + max(height(x->left), height(x->right));
    return x;
}

AVLNode *leftRotate(AVLNode *x)
{
    AVLNode *y = x->right;
    x->right = y->left;
    y->left = x;
    x->height = 1 + max(height(x->left), height(x->right));
    y->height = 1 + max(height(y->left), height(y->right));
    return y;
}

AVLNode *avlInsert(AVLNode *node, int roll)
{
    if (node == NULL)
        return newAVLNode(roll);

    if (roll < node->roll)
        node->left = avlInsert(node->left, roll);
    else
        node->right = avlInsert(node->right, roll);

    node->height = 1 + max(height(node->left), height(node->right));
    int balance = height(node->left) - height(node->right);

    if (balance > 1 && roll < node->left->roll)          /* LL case */
        return rightRotate(node);

    if (balance < -1 && roll > node->right->roll)        /* RR case */
        return leftRotate(node);

    if (balance > 1 && roll > node->left->roll) {        /* LR case */
        node->left = leftRotate(node->left);
        return rightRotate(node);
    }

    if (balance < -1 && roll < node->right->roll) {      /* RL case */
        node->right = rightRotate(node->right);
        return leftRotate(node);
    }

    return node;
}

void inorder(AVLNode *root)              /* prints in roll-number order */
{
    if (root == NULL)
        return;
    inorder(root->left);
    printf("  %d  %s\n", root->roll, students[hashSearch(root->roll)].name);
    inorder(root->right);
}

void showFees(void)
{
    if (feeRoot == NULL) {
        printf("No payments yet.\n");
        return;
    }
    printf("Hostel fee paid by:\n");
    inorder(feeRoot);
    printf("Total collected: Rs %ld  (AVL tree height = %d)\n",
           (long)feeCount * HOSTEL_FEE, height(feeRoot));
}


/* ======================= REGISTER STUDENT ======================= */

void registerStudent(void)
{
    Student s;

    if (studentCount == MAX_STUDENTS) {
        printf("Hostel is full.\n");
        return;
    }

    printf("Roll no: ");
    s.roll = readInt();

    if (s.roll <= 0) {
        printf("Roll no must be a positive number.\n");
        return;
    }

    if (hashSearch(s.roll) != -1) {
        printf("Roll no %d is already registered.\n", s.roll);
        return;
    }

    printf("Hostel fee of Rs %d paid? (y/n): ", HOSTEL_FEE);
    if (!readYesNo()) {
        printf("Fee not paid. Cannot register.\n");
        return;
    }

    do {
        printf("Name: ");
        readText(s.name, sizeof s.name);
    } while (s.name[0] == '\0');        /* name cannot be empty */

    printf("Phone: ");
    readText(s.phone, sizeof s.phone);
    printf("Parent phone: ");
    readText(s.parentPhone, sizeof s.parentPhone);
    printf("Laundry paid this month? (y/n): ");
    s.laundryPaid = readYesNo();
    s.roomNo = 0;

    /* 1. Store in array */
    int index = studentCount;
    students[index] = s;

    /* 2. Put index in its correct place in sortedIdx[] (shift bigger ones right) */
    int i = studentCount - 1;
    while (i >= 0 && students[sortedIdx[i]].roll > s.roll) {
        sortedIdx[i + 1] = sortedIdx[i];
        i--;
    }
    sortedIdx[i + 1] = index;
    studentCount++;

    /* 3. Add to hash table and fee AVL tree */
    hashInsert(s.roll, index);
    feeRoot = avlInsert(feeRoot, s.roll);
    feeCount++;

    printf("Student registered.\n");
}

void printStudent(int i)
{
    if (i == -1) {
        printf("Student not found.\n");
        return;
    }
    printf("Roll: %d | Name: %s | Phone: %s | Parent: %s | Room: ",
           students[i].roll, students[i].name, students[i].phone, students[i].parentPhone);
    if (students[i].roomNo == 0) printf("not allotted\n");
    else                          printf("%d\n", students[i].roomNo);
}

/* Ask for a roll number and return the student's index (or -1) */
int askStudent(void)
{
    printf("Roll no: ");
    int roll = readInt();

    int i = hashSearch(roll);
    if (i == -1)
        printf("Student not found.\n");
    return i;
}


/* ======================= STACK - ROOM ALLOTMENT + UNDO ======================= */

int undoStack[MAX_STUDENTS];     /* stores index of student who got a room */
int top = -1;

void initRooms(void)
{
    int k = 0;
    for (int f = 1; f <= 3; f++)
        for (int r = 1; r <= 4; r++) {
            rooms[k].roomNo = f * 100 + r;
            rooms[k].floor = f;
            rooms[k].occupied = 0;
            k++;
        }
}

int findRoom(int roomNo)
{
    for (int i = 0; i < NUM_ROOMS; i++)
        if (rooms[i].roomNo == roomNo)
            return i;
    return -1;
}

/* Give a room to a student. Returns 1 if allotted, 0 if not. */
int allotRoom(int idx, int roomNo)
{
    int r = findRoom(roomNo);

    if (r == -1) {
        printf("Room not found.\n");
        return 0;
    }
    if (students[idx].roomNo != 0) {
        printf("Already has room %d.\n", students[idx].roomNo);
        return 0;
    }
    if (rooms[r].occupied >= ROOM_CAPACITY) {
        printf("Room %d is full.\n", roomNo);
        return 0;
    }

    students[idx].roomNo = roomNo;
    rooms[r].occupied++;
    undoStack[++top] = idx;                      /* PUSH */

    printf("Room %d given to %s. Beds: %d/%d\n",
           roomNo, students[idx].name, rooms[r].occupied, ROOM_CAPACITY);
    return 1;
}

void undoAllotment(void)
{
    if (top == -1) {
        printf("Nothing to undo.\n");
        return;
    }

    int idx = undoStack[top--];                  /* POP - last allotment first */
    int r = findRoom(students[idx].roomNo);

    if (r != -1 && rooms[r].occupied > 0)
        rooms[r].occupied--;
    printf("Undo: %s removed from room %d.\n", students[idx].name, students[idx].roomNo);
    students[idx].roomNo = 0;
}

void showRooms(void)
{
    for (int i = 0; i < NUM_ROOMS; i++) {
        printf("Room %d  [%d/%d]  ", rooms[i].roomNo, rooms[i].occupied, ROOM_CAPACITY);
        for (int j = 0; j < studentCount; j++)
            if (students[j].roomNo == rooms[i].roomNo)
                printf("%d ", students[j].roll);
        printf("\n");
    }
}


/* ======================= CIRCULAR QUEUE - WAITING LIST ======================= */

int waitQueue[MAX_STUDENTS];
int front = 0, rear = -1, waitCount = 0;

int inQueue(int idx)
{
    for (int i = 0; i < waitCount; i++)
        if (waitQueue[(front + i) % MAX_STUDENTS] == idx)
            return 1;
    return 0;
}

void enqueue(int idx)
{
    if (students[idx].roomNo != 0) { printf("Already has a room.\n"); return; }
    if (inQueue(idx))              { printf("Already in waiting list.\n"); return; }

    rear = (rear + 1) % MAX_STUDENTS;
    waitQueue[rear] = idx;
    waitCount++;
    printf("%s added to waiting list (position %d).\n", students[idx].name, waitCount);
}

int dequeue(void)
{
    if (waitCount == 0)
        return -1;

    int idx = waitQueue[front];
    front = (front + 1) % MAX_STUDENTS;
    waitCount--;
    return idx;
}

void allotFromWaitingList(void)
{
    int freeRoom = -1;
    for (int i = 0; i < NUM_ROOMS; i++)
        if (rooms[i].occupied < ROOM_CAPACITY) {
            freeRoom = i;
            break;
        }

    if (freeRoom == -1) {
        printf("No free beds.\n");
        return;
    }

    int idx = dequeue();
    while (idx != -1 && students[idx].roomNo != 0)   /* skip if they already got a room */
        idx = dequeue();

    if (idx == -1) {
        printf("Waiting list is empty.\n");
        return;
    }

    if (!allotRoom(idx, rooms[freeRoom].roomNo)) {
        /* allotment failed: put the student back at the FRONT so they keep their turn */
        front = (front - 1 + MAX_STUDENTS) % MAX_STUDENTS;
        waitQueue[front] = idx;
        waitCount++;
    }
}


/* ======================= PRIORITY QUEUE - COMPLAINTS ======================= */
/* Sorted linked list: 1 = HIGH comes first. Same priority -> first come first served. */

typedef struct Complaint {
    int id;
    int roomNo;
    char text[100];
    int priority;
    struct Complaint *next;
} Complaint;

Complaint *complaintHead = NULL;
int nextComplaintId = 1;

void addComplaint(int idx)
{
    if (students[idx].roomNo == 0) {
        printf("Student has no room.\n");
        return;
    }

    Complaint *c = malloc(sizeof(Complaint));
    if (c == NULL) {
        printf("Out of memory.\n");
        return;
    }
    c->id = nextComplaintId++;
    c->roomNo = students[idx].roomNo;
    printf("Problem: ");
    readText(c->text, sizeof c->text);
    printf("Priority (1 = HIGH, 2 = MEDIUM, 3 = LOW): ");
    c->priority = readInt();
    if (c->priority < 1 || c->priority > 3)
        c->priority = 3;                         /* treat invalid input as LOW */

    /* insert at the right position */
    if (complaintHead == NULL || c->priority < complaintHead->priority) {
        c->next = complaintHead;
        complaintHead = c;
    } else {
        Complaint *p = complaintHead;
        while (p->next != NULL && p->next->priority <= c->priority)
            p = p->next;
        c->next = p->next;
        p->next = c;
    }
    printf("Complaint #%d filed.\n", c->id);
}

void resolveComplaint(void)
{
    if (complaintHead == NULL) {
        printf("No complaints.\n");
        return;
    }

    Complaint *c = complaintHead;                /* most urgent is always at head */
    complaintHead = complaintHead->next;

    char *level[] = {"", "HIGH", "MEDIUM", "LOW"};
    printf("Resolving #%d [%s] Room %d: %s\n", c->id, level[c->priority], c->roomNo, c->text);
    free(c);
}


/* ======================= LINKED LISTS - GATE ENTRY / EXIT ======================= */

/* Doubly linked list: students currently outside */
typedef struct OutNode {
    int idx;
    char time[20];
    struct OutNode *prev, *next;
} OutNode;

OutNode *outHead = NULL;

/* Singly linked list per student: gate history, newest first */
typedef struct GateLog {
    char type[4];               /* "OUT" or "IN" */
    char time[20];
    struct GateLog *next;
} GateLog;

GateLog *gateHistory[MAX_STUDENTS];

void addGateLog(int idx, char *type)
{
    GateLog *g = malloc(sizeof(GateLog));
    if (g == NULL) {
        printf("Out of memory.\n");
        return;
    }
    strcpy(g->type, type);
    getTime(g->time);
    g->next = gateHistory[idx];         /* insert at head */
    gateHistory[idx] = g;
}

OutNode *findOutside(int idx)
{
    for (OutNode *p = outHead; p != NULL; p = p->next)
        if (p->idx == idx)
            return p;
    return NULL;
}

void goingOut(int idx)
{
    time_t now = time(NULL);
    int hour = localtime(&now)->tm_hour;

    if (hour >= CURFEW_HOUR || hour < 6) { printf("Gate closed (10 PM - 6 AM).\n"); return; }
    if (findOutside(idx) != NULL)        { printf("Already outside.\n"); return; }

    OutNode *node = malloc(sizeof(OutNode));
    if (node == NULL) {
        printf("Out of memory.\n");
        return;
    }
    node->idx = idx;
    getTime(node->time);
    node->prev = NULL;
    node->next = outHead;               /* insert at head */
    if (outHead != NULL)
        outHead->prev = node;
    outHead = node;

    addGateLog(idx, "OUT");
    printf("%s went out. Parent phone: %s. Return before 10 PM.\n",
           students[idx].name, students[idx].parentPhone);
}

void comingIn(int idx)
{
    OutNode *node = findOutside(idx);
    if (node == NULL) {
        printf("Student is not outside.\n");
        return;
    }

    /* remove node using prev and next pointers */
    if (node->prev != NULL) node->prev->next = node->next;
    else                    outHead = node->next;
    if (node->next != NULL) node->next->prev = node->prev;
    free(node);

    addGateLog(idx, "IN");

    time_t now = time(NULL);
    int hour = localtime(&now)->tm_hour;
    if (hour >= CURFEW_HOUR || hour < 6)
        printf("%s is back - LATE ENTRY after curfew.\n", students[idx].name);
    else
        printf("%s is back in the hostel.\n", students[idx].name);
}

void showOutside(void)
{
    if (outHead == NULL) {
        printf("Everyone is inside.\n");
        return;
    }
    for (OutNode *p = outHead; p != NULL; p = p->next)
        printf("  %d  %-15s  out since %s  parent %s\n", students[p->idx].roll,
               students[p->idx].name, p->time, students[p->idx].parentPhone);
}

void showGateHistory(int idx)
{
    if (gateHistory[idx] == NULL) {
        printf("No gate entries.\n");
        return;
    }
    for (GateLog *g = gateHistory[idx]; g != NULL; g = g->next)
        printf("  %-3s  %s\n", g->type, g->time);
}


/* ======================= GRAPH - FIRE EXIT (BFS + DIJKSTRA) ======================= */
/*
   Same layout on every floor (distance in metres):

   Stair A --5-- Room x01 --8-- Room x02 --20-- Stair B --20-- Room x03 --8-- Room x04 --5-- Stair C

   Every stair leads down to an exit.
   From room x02: BFS picks Stair B (fewest steps), Dijkstra picks Stair A (shortest metres).
*/

#define NODES 7
#define INF 99999

char *nodeName[NODES] = {"Stair A", "Room x01", "Room x02", "Stair B",
                         "Room x03", "Room x04", "Stair C"};
int isExit[NODES] = {1, 0, 0, 1, 0, 0, 1};
int graph[NODES][NODES];        /* adjacency matrix, 0 = no corridor */

void addEdge(int a, int b, int dist)
{
    graph[a][b] = dist;
    graph[b][a] = dist;
}

void buildGraph(void)
{
    addEdge(0, 1, 5);
    addEdge(1, 2, 8);
    addEdge(2, 3, 20);
    addEdge(3, 4, 20);
    addEdge(4, 5, 8);
    addEdge(5, 6, 5);
}

/* room 101 -> node 1, 102 -> node 2, 103 -> node 4, 104 -> node 5 */
int roomToNode(int roomNo)
{
    int map[] = {-1, 1, 2, 4, 5};
    int r = roomNo % 100;
    if (findRoom(roomNo) == -1)
        return -1;
    return map[r];
}

void printPath(int parent[], int v)
{
    if (parent[v] != -1) {
        printPath(parent, parent[v]);
        printf(" -> ");
    }
    printf("%s", nodeName[v]);
}

void bfs(int start)
{
    int visited[NODES] = {0}, parent[NODES], queue[NODES];
    int f = 0, r = 0;

    for (int i = 0; i < NODES; i++) parent[i] = -1;
    visited[start] = 1;
    queue[r++] = start;

    while (f < r) {
        int u = queue[f++];
        if (isExit[u]) {
            printf("BFS (fewest steps): ");
            printPath(parent, u);
            printf("\n");
            return;
        }
        for (int v = 0; v < NODES; v++)
            if (graph[u][v] != 0 && !visited[v]) {
                visited[v] = 1;
                parent[v] = u;
                queue[r++] = v;
            }
    }
    printf("No exit reachable!\n");
}

void dijkstra(int start)
{
    int dist[NODES], done[NODES] = {0}, parent[NODES];

    for (int i = 0; i < NODES; i++) {
        dist[i] = INF;
        parent[i] = -1;
    }
    dist[start] = 0;

    for (int k = 0; k < NODES; k++) {
        int u = -1;
        for (int i = 0; i < NODES; i++)          /* pick nearest unvisited node */
            if (!done[i] && (u == -1 || dist[i] < dist[u]))
                u = i;

        if (u == -1 || dist[u] == INF)           /* nothing reachable is left */
            break;
        done[u] = 1;

        for (int v = 0; v < NODES; v++)          /* relax neighbours */
            if (graph[u][v] != 0 && dist[u] + graph[u][v] < dist[v]) {
                dist[v] = dist[u] + graph[u][v];
                parent[v] = u;
            }
    }

    int best = -1;
    for (int i = 0; i < NODES; i++)
        if (isExit[i] && dist[i] != INF && (best == -1 || dist[i] < dist[best]))
            best = i;

    if (best == -1) {
        printf("No exit reachable!\n");
        return;
    }

    printf("Dijkstra (shortest walk): ");
    printPath(parent, best);
    printf("  = %d m\n", dist[best]);
}


/* ======================= SORTING ======================= */

int comparisons;

void insertionSort(Student a[], int n)
{
    for (int i = 1; i < n; i++) {
        Student key = a[i];
        int j = i - 1;
        while (j >= 0 && (comparisons++, a[j].roll > key.roll)) {
            a[j + 1] = a[j];
            j--;
        }
        a[j + 1] = key;
    }
}

void selectionSort(Student a[], int n)
{
    for (int i = 0; i < n - 1; i++) {
        int min = i;
        for (int j = i + 1; j < n; j++) {
            comparisons++;
            if (a[j].roll < a[min].roll)
                min = j;
        }
        Student t = a[i]; a[i] = a[min]; a[min] = t;
    }
}

void merge(Student a[], int l, int m, int r)
{
    Student temp[MAX_STUDENTS];
    int i = l, j = m + 1, k = 0;

    while (i <= m && j <= r) {
        comparisons++;
        if (a[i].roll <= a[j].roll) temp[k++] = a[i++];
        else                        temp[k++] = a[j++];
    }
    while (i <= m) temp[k++] = a[i++];
    while (j <= r) temp[k++] = a[j++];

    for (k = 0; k < r - l + 1; k++)
        a[l + k] = temp[k];
}

void mergeSort(Student a[], int l, int r)
{
    if (l >= r)
        return;
    int m = (l + r) / 2;
    mergeSort(a, l, m);
    mergeSort(a, m + 1, r);
    merge(a, l, m, r);
}

void compareSorts(void)
{
    Student copy[MAX_STUDENTS];       /* sort a copy so indices in students[] stay the same */

    if (studentCount == 0) {
        printf("No students registered.\n");
        return;
    }

    memcpy(copy, students, sizeof(Student) * studentCount);
    comparisons = 0;
    insertionSort(copy, studentCount);
    printf("Insertion sort comparisons: %d\n", comparisons);

    memcpy(copy, students, sizeof(Student) * studentCount);
    comparisons = 0;
    selectionSort(copy, studentCount);
    printf("Selection sort comparisons: %d\n", comparisons);

    memcpy(copy, students, sizeof(Student) * studentCount);
    comparisons = 0;
    mergeSort(copy, 0, studentCount - 1);
    printf("Merge sort comparisons:     %d\n", comparisons);

    printf("Sorted by roll no:\n");
    for (int i = 0; i < studentCount; i++)
        printf("  %d  %s\n", copy[i].roll, copy[i].name);
}


/* ======================= LAUNDRY ======================= */

void showLaundry(void)
{
    int paid = 0;
    for (int i = 0; i < studentCount; i++) {
        printf("  %d  %-15s %s\n", students[i].roll, students[i].name,
               students[i].laundryPaid ? "paid" : "not paid");
        paid += students[i].laundryPaid;
    }
    printf("%d students paid, Rs %d collected.\n", paid, paid * LAUNDRY_FEE);
}


/* ======================= MAIN MENU ======================= */

int main(void)
{
    int choice, idx, n;

    initRooms();
    buildGraph();

    do {
        printf("\n===== HOSTEL MANAGEMENT SYSTEM =====\n");
        printf(" 1. Register student          2. Linear search\n");
        printf(" 3. Binary search             4. Hash search\n");
        printf(" 5. Allot room                6. Undo last allotment\n");
        printf(" 7. Add to waiting list       8. Allot from waiting list\n");
        printf(" 9. View rooms               10. File complaint\n");
        printf("11. Resolve complaint        12. View fee payments\n");
        printf("13. Pay laundry              14. View laundry\n");
        printf("15. Student going out        16. Student coming in\n");
        printf("17. Students outside         18. Gate history\n");
        printf("19. Fire exit (BFS)          20. Fire exit (Dijkstra)\n");
        printf("21. Compare sorting           0. Exit\n");
        printf("Choice: ");

        choice = readInt();

        switch (choice) {
        case 1:  registerStudent(); break;

        case 2: case 3: case 4:
            printf("Roll no: ");
            n = readInt();
            if (choice == 2)      printStudent(linearSearch(n));
            else if (choice == 3) printStudent(binarySearch(n));
            else                  printStudent(hashSearch(n));
            break;

        case 5:
            if ((idx = askStudent()) != -1) {
                printf("Room no: ");
                n = readInt();
                allotRoom(idx, n);
            }
            break;

        case 6:  undoAllotment(); break;
        case 7:  if ((idx = askStudent()) != -1) enqueue(idx); break;
        case 8:  allotFromWaitingList(); break;
        case 9:  showRooms(); break;
        case 10: if ((idx = askStudent()) != -1) addComplaint(idx); break;
        case 11: resolveComplaint(); break;
        case 12: showFees(); break;

        case 13:
            if ((idx = askStudent()) != -1) {
                if (students[idx].laundryPaid) {
                    printf("Laundry already paid this month.\n");
                } else {
                    students[idx].laundryPaid = 1;
                    printf("Laundry paid: Rs %d for 75 clothes.\n", LAUNDRY_FEE);
                }
            }
            break;

        case 14: showLaundry(); break;
        case 15: if ((idx = askStudent()) != -1) goingOut(idx); break;
        case 16: if ((idx = askStudent()) != -1) comingIn(idx); break;
        case 17: showOutside(); break;
        case 18: if ((idx = askStudent()) != -1) showGateHistory(idx); break;

        case 19: case 20: {
            printf("Room no: ");
            n = readInt();
            int node = roomToNode(n);
            if (node == -1)        printf("Room not found.\n");
            else if (choice == 19) bfs(node);
            else                   dijkstra(node);
            break;
        }

        case 21: compareSorts(); break;
        case 0:  printf("Bye!\n"); freeMemory(); break;
        default: printf("Invalid choice.\n");
        }
    } while (choice != 0);

    return 0;
}


/* ======================= FREE MEMORY ======================= */
/* Free every node we created with malloc before the program ends */

void freeAVL(AVLNode *root)
{
    if (root == NULL)
        return;
    freeAVL(root->left);
    freeAVL(root->right);
    free(root);
}

void freeMemory(void)
{
    for (int i = 0; i < HASH_SIZE; i++) {           /* hash table chains */
        HashNode *p = hashTable[i];
        while (p != NULL) {
            HashNode *next = p->next;
            free(p);
            p = next;
        }
        hashTable[i] = NULL;
    }

    freeAVL(feeRoot);                               /* fee tree */
    feeRoot = NULL;

    while (complaintHead != NULL) {                 /* complaints */
        Complaint *next = complaintHead->next;
        free(complaintHead);
        complaintHead = next;
    }

    while (outHead != NULL) {                       /* students outside */
        OutNode *next = outHead->next;
        free(outHead);
        outHead = next;
    }

    for (int i = 0; i < studentCount; i++) {        /* gate history */
        GateLog *g = gateHistory[i];
        while (g != NULL) {
            GateLog *next = g->next;
            free(g);
            g = next;
        }
        gateHistory[i] = NULL;
    }
}