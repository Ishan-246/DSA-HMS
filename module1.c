
/* ================================================================
   MODULE 3: Queues — Complaints & Room Waiting List (Unit 3)
   ================================================================ */

/* ---- Plain circular queue: room allotment waiting list ---- */

int waitingList[MAX_STUDENTS];   /* roll numbers */
int wlFront = 0, wlRear = -1, wlCount = 0;

/* Check full (wlCount >= MAX_STUDENTS) first.
   wlRear = (wlRear + 1) % MAX_STUDENTS; store; wlCount++. */
void enqueue_waiting(int rollNo);

/* Check empty (wlCount == 0) first. Read at wlFront,
   wlFront = (wlFront + 1) % MAX_STUDENTS, wlCount--, return it.
   Return -1 on empty so the caller can detect it. */
int dequeue_waiting(void);


/* ---- Priority queue: complaints by urgency ----
   Simplest correct approach for a DSA course: keep the array
   sorted on insert (insertion-sort style). Demonstrates the
   concept without needing a full binary heap.
   A burst pipe (HIGH) jumps ahead of a flickering bulb (LOW)
   even if the bulb was reported first — good viva talking point. */

Complaint complaintQueue[MAX_COMPLAINTS];
int complaintCount = 0;

/* Insert c keeping ascending priority order (index 0 = most
   urgent). Shift elements right to open the correct slot. */
void complaint_enqueue(Complaint c);

/* Remove and return complaintQueue[0]. Shift everything left
   by one, decrement complaintCount. This is what the warden
   assigns to staff next. */
Complaint complaint_dequeue(void);


/* ================================================================
   MODULE 4: Linked Lists (Unit 4)
   ================================================================ */

/* ---- Doubly linked list: per-student gate entry/exit log ----
   Doubly linked so the warden can walk backwards from the most
   recent entry — which is what you'd actually do when checking
   "when did this student last leave?" */

typedef struct GateNode {
    char type[10];       /* "IN" or "OUT" */
    int timestamp;       /* simple incrementing counter is fine */
    struct GateNode* prev;
    struct GateNode* next;
} GateNode;

GateNode* gateHead[MAX_STUDENTS];
GateNode* gateTail[MAX_STUDENTS];

/* malloc a node, append to the END of that student's list:
   - empty list: node becomes both head and tail
   - otherwise: node->prev = old tail; oldTail->next = node;
     tail = node */
void log_gate_entry(int studentIndex, const char* type, int timestamp);

/* Walk backwards from gateTail via ->prev, printing each node.
   Show this in your demo — it's the reason you chose DOUBLY
   linked over singly, and examiners ask exactly that. */
void print_gate_log_recent_first(int studentIndex);


/* ---- Singly linked list: staff duty roster per floor ----
   Floors gain/lose staff dynamically, so a linked list beats a
   fixed array here. Justify that choice in your report. */

typedef struct DutyNode {
    int staffId;
    struct DutyNode* next;
} DutyNode;

DutyNode* floorDuty[10];    /* floorDuty[f] = staff on floor f */

/* malloc a node, prepend to floorDuty[floor] (O(1) insert —
   no need to walk the list). */
void assign_staff_to_floor(int floor, int staffId);

/* Walk the chain, unlink the node with matching staffId, free it.
   Handle the head-node case separately — classic linked list
   deletion, and a very likely viva question. */
void remove_staff_from_floor(int floor, int staffId);


/* ================================================================
   MODULE 5: Hashing — O(1) Student Lookup (Unit 5)
   ================================================================
   Map rollNo -> index in students[], so the warden's most common
   operation (look up a student) doesn't need an O(n) scan.
   Chaining handles collisions.
*/

typedef struct HashNode {
    int rollNo;
    int studentIndex;
    struct HashNode* next;    /* chaining */
} HashNode;

HashNode* hashTable[HASH_SIZE];

/* rollNo % HASH_SIZE. In your report explain why a prime (101)
   is chosen over a round number like 100 — roll numbers often
   share patterns (year prefix, branch code), and a round modulus
   makes them collide far more. */
int hash_function(int rollNo);

/* malloc a HashNode, insert at the HEAD of the bucket's chain. */
void hash_insert(int rollNo, int studentIndex);

/* Hash to the bucket, walk the chain comparing rollNo.
   Return studentIndex, or -1 if absent.
   Demo this next to linear_search_student() on the same data —
   the contrast is your strongest Unit 5 talking point. */
int hash_lookup(int rollNo);


/* ================================================================
   MODULE 6: Binary Search Tree — Students by Fee Due Date (Unit 6)
   ================================================================
   In-order traversal gives students in ascending due-date order
   for free — exactly the "who owes fees soonest" list a warden
   needs, with no sorting step.
*/

typedef struct BSTNode {
    Student student;
    struct BSTNode* left;
    struct BSTNode* right;
} BSTNode;

BSTNode* feeTreeRoot = NULL;

/* Recursive insert keyed on feeDueDay: smaller goes left, larger
   or equal goes right. Base case: NULL root -> malloc a new node
   and return it. Call as: root = bst_insert(root, s); */
BSTNode* bst_insert(BSTNode* root, Student s);

/* In-order traversal (left, root, right) = ascending due date.
   Satisfies the syllabus's tree traversal requirement directly. */
void bst_inorder(BSTNode* root);

/* Print only students with feesPaid == 0 during traversal —
   "pending fees, most urgent first". Same traversal, filtered. */
void bst_print_pending_fees(BSTNode* root);

/* Optional stretch, only once the plain BST works: AVL rotations
   on insert to keep it balanced. Stronger grade, not required. */


/* ================================================================
   MODULE 7: Graph — Hostel Floor Plan (Unit 7)
   ================================================================
   Model the building as an undirected weighted graph:
     nodes    = rooms, corridor junctions, stairwells, exits
     edges    = physical connections
     weights  = walking distance in metres

   This is your strongest demo. Two things to show:
     1. BFS  -> fewest-hops route from a room to the nearest exit
     2. Dijkstra -> shortest route by actual DISTANCE (weighted)
   Then explain WHY they can differ: fewest doors passed through
   isn't always the shortest walk. That contrast is a genuinely
   good viva answer.

   Draw your actual hostel floor on paper, number the nodes, and
   hardcode the edges. Graders love seeing a real building mapped.
*/

typedef struct AdjNode {
    int nodeIndex;
    int distance;          /* edge weight in metres */
    struct AdjNode* next;
} AdjNode;

AdjNode* floorGraph[MAX_NODES];
char nodeNames[MAX_NODES][MAX_NAME];   /* "Room 101", "Stair A", "Main Exit" */
int isExit[MAX_NODES];                 /* 1 if this node is an exit */
int graphNodeCount = 0;

/* Undirected: malloc and prepend a node to BOTH floorGraph[a]
   and floorGraph[b]. Forgetting the second one is the classic
   bug here — the path works one direction only. */
void add_corridor(int a, int b, int distance);

/* BFS from startNode:
   - visited[] array + a queue of node indices
   - parent[] array to reconstruct the path afterwards
   - stop when you dequeue a node with isExit == 1
   - walk parent[] backwards from that exit to print the route
   Unweighted: counts doors/junctions passed, not metres. */
void bfs_nearest_exit(int startNode);

/* Dijkstra from startNode:
   - dist[] initialised to a large value, dist[start] = 0
   - repeatedly pick the unvisited node with smallest dist
   - relax each neighbour: if dist[u] + weight < dist[v],
     update dist[v] and parent[v]
   - among all exit nodes, report the one with smallest dist
   Weighted: gives the genuinely shortest walk in metres. */
void dijkstra_nearest_exit(int startNode);

/* DFS from startNode marking visited[].
   Use case: if a corridor is blocked (remove that edge), which
   rooms can still reach an exit? Any unvisited room is cut off.
   Good extension if you have time. */
void dfs_reachable(int startNode, int visited[]);


/* ================================================================
   MODULE 8: Sorting — Compare Two Algorithms (Unit 8)
   ================================================================
   Sort students[] by rollNo (needed anyway for binary search) or
   by feeDueDay. Implement BOTH, count comparisons and swaps on
   the SAME dataset, and put the numbers in your report.
   Insertion Sort O(n^2) vs Merge Sort O(n log n) is the clearest
   contrast — the gap widens visibly as n grows, so run it at
   n = 10, 50, 200 and tabulate.
*/

long insertionComparisons = 0, insertionSwaps = 0;
long mergeComparisons = 0;

void insertion_sort_students(void);
void merge_sort_students(int left, int right);
void merge(int left, int mid, int right);


/* ================================================================
   MAIN MENU
   ================================================================ */

void print_menu(void) {
    printf("\n===== HOSTEL MANAGEMENT SYSTEM (Warden Console) =====\n");
    printf("--- Students ---\n");
    printf("1.  Register Student\n");
    printf("2.  Search Student (linear)\n");
    printf("3.  Search Student (hash table)\n");
    printf("--- Rooms ---\n");
    printf("4.  Allot Room\n");
    printf("5.  Undo Last Allotment\n");
    printf("6.  Add to Room Waiting List\n");
    printf("7.  Allot Next from Waiting List\n");
    printf("--- Complaints ---\n");
    printf("8.  File Complaint (priority queue)\n");
    printf("9.  Resolve Next Complaint (most urgent first)\n");
    printf("--- Staff ---\n");
    printf("10. Assign Staff to Floor\n");
    printf("11. Remove Staff from Floor\n");
    printf("12. View Floor Duty Roster\n");
    printf("--- Fees ---\n");
    printf("13. View Students by Fee Due Date (BST in-order)\n");
    printf("14. View Pending Fees Only\n");
    printf("--- Gate Log ---\n");
    printf("15. Record Gate Entry/Exit\n");
    printf("16. View Student Gate Log (most recent first)\n");
    printf("--- Safety ---\n");
    printf("17. Nearest Exit from Room (BFS - fewest junctions)\n");
    printf("18. Nearest Exit from Room (Dijkstra - shortest metres)\n");
    printf("--- Analysis ---\n");
    printf("19. Sort Students (insertion vs merge, compare counts)\n");
    printf("20. Exit\n");
    printf("Choice: ");
}

int main(void) {
    int choice;

    /* TODO: initialise the floor graph here — add_corridor() calls
       for your hardcoded hostel layout, fill nodeNames[] and
       isExit[]. Do this once at startup before the menu loop. */

    do {
        print_menu();
        scanf("%d", &choice);

        switch (choice) {
            case 1:  /* TODO: prompt for rollNo/name/feeDueDay, build a Student,
                        add_student(), then hash_insert() and bst_insert() on it */ break;
            case 2:  /* TODO: prompt rollNo, linear_search_student(), print or "not found" */ break;
            case 3:  /* TODO: prompt rollNo, hash_lookup(), print or "not found" */ break;
            case 4:  /* TODO: prompt rollNo + roomNo. Check room has space
                        (occupied < capacity). push_undo() with CURRENT roomNo
                        BEFORE changing it. Update student.roomNo and
                        rooms[].occupied. */ break;
            case 5:  undo_last_allotment(); break;
            case 6:  /* TODO: prompt rollNo, enqueue_waiting() */ break;
            case 7:  /* TODO: dequeue_waiting(), handle -1 (empty), then allot */ break;
            case 8:  /* TODO: prompt details + priority, build a Complaint,
                        complaint_enqueue() */ break;
            case 9:  /* TODO: complaint_dequeue(), handle empty, mark resolved,
                        print which staff member on that floor handles it */ break;
            case 10: /* TODO: prompt floor + staffId, assign_staff_to_floor() */ break;
            case 11: /* TODO: prompt floor + staffId, remove_staff_from_floor() */ break;
            case 12: /* TODO: prompt floor, walk floorDuty[floor] printing each */ break;
            case 13: bst_inorder(feeTreeRoot); break;
            case 14: bst_print_pending_fees(feeTreeRoot); break;
            case 15: /* TODO: prompt student + IN/OUT, log_gate_entry() */ break;
            case 16: /* TODO: prompt student, print_gate_log_recent_first() */ break;
            case 17: /* TODO: prompt room node index, bfs_nearest_exit() */ break;
            case 18: /* TODO: prompt room node index, dijkstra_nearest_exit() */ break;
            case 19: /* TODO: run both sorts on COPIES of students[], print
                        comparison/swap counts side by side */ break;
            case 20: printf("Exiting...\n"); break;
            default: printf("Invalid choice.\n");
        }
    } while (choice != 20);

    /* TODO before returning: free every malloc'd node.
       Write one free routine per module (gate logs, duty rosters,
       hash chains, BST, graph adjacency lists) and call them from
       a single free_all(). Do NOT leave this to the end of the
       project — add each free routine as you finish its module. */

    return 0;
}
