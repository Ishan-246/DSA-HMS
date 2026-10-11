#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define MAX_STUDENTS 200
#define MAX_ROOMS 100
#define MAX_STAFF 50
#define MAX_COMPLAINTS 100
#define HASH_SIZE 101
#define MAX_NAME 50
#define ROLL_LEN 12          /* e.g. "N254" */
#define PHONE_LEN 11         /* 10 digits + '\0' */
#define MAX_UNDO 100
#define MAX_NODES 120

#define NUM_FLOORS 3         /* floors 1-3; ground floor has no rooms */
#define ROOMS_PER_FLOOR 4
#define ROOM_CAPACITY 4

#define HOSTEL_FEE 100000    /* Rs per academic year, due in July */
#define LAUNDRY_FEE 500      /* Rs per month */
#define LAUNDRY_CLOTHES 75   /* clothes per month */
#define GATE_OPEN_HOUR 6     /* gate reopens at 6 AM after curfew */
#define CLEAN_HISTORY 30     /* cleaning records kept per room */
#define PHONE_TABLE_SIZE 1009 /* prime, > 3 phones x MAX_STUDENTS */
#define MAX_EXPR 200
#define MAX_NOTICE 200

#define INF 1000000000

/* ================================================================
   CORE ENTITIES
   ================================================================ */

typedef struct {
    char rollNo[ROLL_LEN];
    char name[MAX_NAME];
    char phone[PHONE_LEN];
    char parentPhone1[PHONE_LEN];
    char parentPhone2[PHONE_LEN];
    int roomNo;          /* 0 = not allotted */
    int hostelFeePaid;   /* 0 = pending, 1 = paid */
    time_t feePaidOn;
    int laundryMonth;    /* YYYYMM of last laundry payment, 0 = never */
} Student;

typedef struct {
    int roomNo;
    int capacity;
    int occupied;
    int floor;
} Room;

typedef enum {
    ROLE_SWEEPER,
    ROLE_ELECTRICIAN,
    ROLE_PLUMBER,
    ROLE_SECURITY
} StaffRole;

typedef struct {
    int staffId;
    char name[MAX_NAME];
    StaffRole role;
    int assignedFloor;   /* 0 = not on duty */
} Staff;

typedef enum {
    COMP_LOW = 3,
    COMP_MEDIUM = 2,
    COMP_HIGH = 1
} Priority;

typedef struct {
    int complaintId;
    int studentIndex;
    int roomNo;
    char description[100];
    Priority priority;
    int resolved;
} Complaint;


/* ================================================================
   GLOBAL STORAGE
   ================================================================
   Students are never removed or reordered in students[], so an
   index into it stays valid forever. The hash table, BST, gate
   logs, queues and roll-number order all store these indices.
*/

Student students[MAX_STUDENTS];
int studentCount = 0;

Room rooms[MAX_ROOMS];
int roomCount = 0;

Staff staff[MAX_STAFF];
int staffCount = 0;

/* Set once at startup: boys hostel = 10 PM, girls hostel = 9 PM */
int curfewHour = 22;
const char* hostelType = "Boys";


/* ================================================================
   FUNCTION PROTOTYPES
   ================================================================ */

/* Input / utility helpers */
void read_line(const char* prompt, char* buf, int size);
int read_int(const char* prompt);
int read_int_range(const char* prompt, int min, int max);
void read_roll(const char* prompt, char* roll);
void read_phone(const char* prompt, char* phone);
int compare_roll(const char* a, const char* b);
void format_time(time_t t, char* buf, int size);
int current_month(void);

/* Module 1 */
int linear_search_student(const char* rollNo);
int binary_search_student(const char* rollNo);
int add_student(Student s);
void print_student(int index);

/* Module 2 */
void initialize_rooms(void);
int find_room(int roomNo);
void push_undo(int studentIndex, int previousRoomNo);
int allot_room(int studentIndex, int roomNo);
void undo_last_allotment(void);
void print_rooms(void);

/* Module 3 */
void enqueue_waiting(int studentIndex);
int dequeue_waiting(void);
int is_in_waiting_list(int studentIndex);
void complaint_enqueue(Complaint c);
Complaint complaint_dequeue(void);
void print_all_complaints(void);
const char* priority_name(Priority p);
void record_cleaning(int studentIndex);
void print_cleaning_record(int roomNo);

/* Module 4 */
void log_gate_entry(int studentIndex, const char* type, time_t timestamp);
void print_gate_log_recent_first(int studentIndex);
void going_out(int studentIndex);
void coming_in(int studentIndex);
void print_students_outside(void);
void initialize_staff(void);
int find_staff(int staffId);
void assign_staff_to_floor(int floor, int staffId);
void remove_staff_from_floor(int floor, int staffId);
void print_floor_duty(int floor);

/* Module 5 */
int hash_function(const char* rollNo);
void hash_insert(const char* rollNo, int studentIndex);
int hash_lookup(const char* rollNo);
void phone_insert(const char* phone, int studentIndex, int whose);
void find_by_phone(const char* phone);

/* Module 6 */
typedef struct AVLNode AVLNode;
AVLNode* avl_insert(AVLNode* root, int studentIndex);
AVLNode* avl_delete(AVLNode* root, int studentIndex);
void avl_inorder(AVLNode* root);

/* Module 7 */
void build_floor_graph(void);
void add_corridor(int a, int b, int distance);
int room_to_node(int roomNo);
void bfs_nearest_exit(int startNode);
void dijkstra_nearest_exit(int startNode);
void dfs_reachable(int startNode, int visited[]);
void check_blocked_corridors(void);

/* Module 8 */
void insertion_sort_students(Student a[], int n);
void merge_sort_students(Student a[], Student temp[], int left, int right);
void merge(Student a[], Student temp[], int left, int mid, int right);
void compare_sorts(void);

void free_all(void);


/* ================================================================
   INPUT / UTILITY HELPERS
   ================================================================
   Everything is read a whole line at a time with fgets(). Plain
   scanf("%d") loops forever if the user types a letter, because
   the bad character is never consumed.
*/

void read_line(const char* prompt, char* buf, int size)
{
    printf("%s", prompt);

    if (fgets(buf, size, stdin) == NULL)
    {
        printf("\nInput closed. Exiting.\n");
        free_all();
        exit(0);
    }

    /* Drop a UTF-8 byte-order mark (PowerShell pipes / Notepad files) */
    if ((unsigned char)buf[0] == 0xEF && (unsigned char)buf[1] == 0xBB &&
        (unsigned char)buf[2] == 0xBF)
    {
        memmove(buf, buf + 3, strlen(buf + 3) + 1);
    }

    size_t len = strlen(buf);

    if (len > 0 && buf[len - 1] == '\n')
    {
        buf[--len] = '\0';

        /* Windows line ending "\r\n" */
        if (len > 0 && buf[len - 1] == '\r')
        {
            buf[--len] = '\0';
        }
    }
    else
    {
        /* Line was longer than buf: discard the rest of it */
        int c;
        while ((c = getchar()) != '\n' && c != EOF)
            ;
    }
}


int read_int(const char* prompt)
{
    char buf[64];

    while (1)
    {
        read_line(prompt, buf, sizeof buf);

        char* end;
        long value = strtol(buf, &end, 10);

        if (end != buf)
        {
            while (*end == ' ' || *end == '\t')
            {
                end++;
            }

            if (*end == '\0')
            {
                return (int)value;
            }
        }

        printf("Please enter a valid number.\n");
    }
}


int read_int_range(const char* prompt, int min, int max)
{
    while (1)
    {
        int value = read_int(prompt);

        if (value >= min && value <= max)
        {
            return value;
        }

        printf("Please enter a number from %d to %d.\n", min, max);
    }
}


/*
   Roll numbers look like "N254": one or more letters followed by
   one or more digits. Stored in uppercase.
*/
void read_roll(const char* prompt, char* roll)
{
    char buf[64];

    while (1)
    {
        read_line(prompt, buf, sizeof buf);

        int i = 0;

        while (isalpha((unsigned char)buf[i]))
        {
            buf[i] = (char)toupper((unsigned char)buf[i]);
            i++;
        }

        int letters = i;

        while (isdigit((unsigned char)buf[i]))
        {
            i++;
        }

        if (letters > 0 && i > letters && buf[i] == '\0' && i < ROLL_LEN)
        {
            strcpy(roll, buf);
            return;
        }

        printf("Roll no must look like N254 (letters then digits).\n");
    }
}


void read_phone(const char* prompt, char* phone)
{
    char buf[64];

    while (1)
    {
        read_line(prompt, buf, sizeof buf);

        int ok = (strlen(buf) == PHONE_LEN - 1);

        for (int i = 0; ok && buf[i] != '\0'; i++)
        {
            if (!isdigit((unsigned char)buf[i]))
            {
                ok = 0;
            }
        }

        if (ok)
        {
            strcpy(phone, buf);
            return;
        }

        printf("Phone number must be exactly 10 digits.\n");
    }
}


/*
   Compare roll numbers: letter part alphabetically, then number
   part numerically, so N9 < N10 < N254 (plain strcmp would put
   "N10" before "N9").

   Returns <0, 0 or >0 like strcmp.
*/
int compare_roll(const char* a, const char* b)
{
    int i = 0;

    while (isalpha((unsigned char)a[i]) && a[i] == b[i])
    {
        i++;
    }

    if (isalpha((unsigned char)a[i]) || isalpha((unsigned char)b[i]))
    {
        return (unsigned char)a[i] - (unsigned char)b[i];
    }

    long na = atol(a + i);
    long nb = atol(b + i);

    return (na > nb) - (na < nb);
}


void format_time(time_t t, char* buf, int size)
{
    strftime(buf, size, "%d-%m-%Y %I:%M %p", localtime(&t));
}


/* Current month as YYYYMM, e.g. 202610 */
int current_month(void)
{
    time_t now = time(NULL);
    struct tm* t = localtime(&now);

    return (t->tm_year + 1900) * 100 + (t->tm_mon + 1);
}


int same_day(time_t a, time_t b)
{
    struct tm ta = *localtime(&a);
    struct tm tb = *localtime(&b);

    return ta.tm_year == tb.tm_year && ta.tm_yday == tb.tm_yday;
}


/* ================================================================
   MODULE 1: ARRAYS + SEARCH
   ================================================================ */

/*
   rollOrder[] holds indices into students[], kept sorted by roll
   number. A new student is placed in its correct slot by shifting
   larger entries one place right (array insertion), so binary
   search always works no matter what order students register in.
*/
int rollOrder[MAX_STUDENTS];


/*
   Add a student to the students array, then index them in the
   roll-number order and the hash table.

   Returns:
   1 -> student successfully added
   0 -> student not added
*/
int add_student(Student s)
{
    /* Check for duplicate roll number */
    if (hash_lookup(s.rollNo) != -1)
    {
        printf("Student with roll number %s already exists.\n",
               s.rollNo);
        return 0;
    }

    /* Check array capacity */
    if (studentCount >= MAX_STUDENTS)
    {
        printf("Student limit reached.\n");
        return 0;
    }

    /* Add student */
    int index = studentCount;
    students[index] = s;

    /* Array insertion into rollOrder[] - O(n) shifting */
    int i = studentCount - 1;

    while (i >= 0 && compare_roll(students[rollOrder[i]].rollNo, s.rollNo) > 0)
    {
        rollOrder[i + 1] = rollOrder[i];
        i--;
    }

    rollOrder[i + 1] = index;

    studentCount++;

    hash_insert(s.rollNo, index);
    phone_insert(s.phone, index, 0);
    phone_insert(s.parentPhone1, index, 1);
    phone_insert(s.parentPhone2, index, 2);

    return 1;
}


/*
   Linear Search

   Searches every student one by one.

   Returns:
   index -> if student is found
   -1    -> if student is not found

   Time Complexity: O(n)
*/
int linear_search_student(const char* rollNo)
{
    for (int i = 0; i < studentCount; i++)
    {
        if (strcmp(students[i].rollNo, rollNo) == 0)
        {
            return i;
        }
    }

    return -1;
}


/*
   Binary Search

   Searches rollOrder[], which is always sorted by roll number.

   Returns:
   index into students[] -> if student is found
   -1                    -> if student is not found

   Time Complexity: O(log n)
*/
int binary_search_student(const char* rollNo)
{
    int left = 0;
    int right = studentCount - 1;

    while (left <= right)
    {
        int mid = left + (right - left) / 2;
        int cmp = compare_roll(students[rollOrder[mid]].rollNo, rollNo);

        /* Student found */
        if (cmp == 0)
        {
            return rollOrder[mid];
        }

        /* Search right half */
        if (cmp < 0)
        {
            left = mid + 1;
        }

        /* Search left half */
        else
        {
            right = mid - 1;
        }
    }

    return -1;
}


/* Defined in Module 4 */
int is_outside(int studentIndex);


void print_student(int index)
{
    if (index == -1)
    {
        printf("Student not found.\n");
        return;
    }

    Student* s = &students[index];

    printf("Student found!\n");
    printf("Roll no:        %s\n", s->rollNo);
    printf("Name:           %s\n", s->name);
    printf("Phone:          %s\n", s->phone);
    printf("Parent phone 1: %s\n", s->parentPhone1);
    printf("Parent phone 2: %s\n", s->parentPhone2);

    if (s->roomNo == 0)
    {
        printf("Room:           not allotted\n");
    }
    else
    {
        printf("Room:           %d\n", s->roomNo);
    }

    printf("Hostel fee:     %s\n", s->hostelFeePaid ? "paid" : "PENDING");
    printf("Laundry:        %s this month\n",
           s->laundryMonth == current_month() ? "active" : "not taken");
    printf("Currently:      %s\n", is_outside(index) ? "OUTSIDE campus" : "in hostel");
}


/* ================================================================
   MODULE 2: STACK + ROOM ALLOTMENT + UNDO
   ================================================================ */

/*
   Each stack entry stores one room allotment action.
*/
typedef struct
{
    int studentIndex;
    int previousRoomNo;
} AllotmentAction;


/*
   Undo Stack

   undoTop = -1 means stack is empty.
*/
AllotmentAction undoStack[MAX_UNDO];

int undoTop = -1;


/*
   Initialize hostel rooms.

   Floors 1-3 (ground floor has no student rooms), 4 rooms per
   floor: 101-104, 201-204, 301-304. Each room has 4 beds.
*/
void initialize_rooms(void)
{
    roomCount = 0;

    for (int f = 1; f <= NUM_FLOORS; f++)
    {
        for (int r = 1; r <= ROOMS_PER_FLOOR; r++)
        {
            rooms[roomCount].roomNo = f * 100 + r;
            rooms[roomCount].capacity = ROOM_CAPACITY;
            rooms[roomCount].occupied = 0;
            rooms[roomCount].floor = f;
            roomCount++;
        }
    }
}


/*
   Find a room using its room number.

   Returns:
   index -> if room is found
   -1    -> if room is not found
*/
int find_room(int roomNo)
{
    for (int i = 0; i < roomCount; i++)
    {
        if (rooms[i].roomNo == roomNo)
        {
            return i;
        }
    }

    return -1;
}


/*
   First room that still has a free bed, or -1 if all are full.
*/
int find_free_room(void)
{
    for (int i = 0; i < roomCount; i++)
    {
        if (rooms[i].occupied < rooms[i].capacity)
        {
            return i;
        }
    }

    return -1;
}


void print_rooms(void)
{
    printf("Room  Floor  Beds taken  Students\n");

    for (int i = 0; i < roomCount; i++)
    {
        printf("%-5d %-6d %d/%d         ", rooms[i].roomNo, rooms[i].floor,
               rooms[i].occupied, rooms[i].capacity);

        for (int j = 0; j < studentCount; j++)
        {
            if (students[j].roomNo == rooms[i].roomNo)
            {
                printf("%s ", students[j].rollNo);
            }
        }

        printf("\n");
    }
}


/*
   Push an allotment action onto the undo stack.

   Stack follows LIFO:
   Last In -> First Out
*/
void push_undo(int studentIndex, int previousRoomNo)
{
    /* Check stack overflow */
    if (undoTop >= MAX_UNDO - 1)
    {
        printf("Undo stack is full.\n");
        return;
    }

    /* Move top upward */
    undoTop++;

    /* Store action */
    undoStack[undoTop].studentIndex = studentIndex;
    undoStack[undoTop].previousRoomNo = previousRoomNo;
}


/*
   Allot a room to a student.

   Returns:
   1 -> room allotted
   0 -> not allotted
*/
int allot_room(int studentIndex, int roomNo)
{
    /* Find room */
    int roomIndex = find_room(roomNo);

    if (roomIndex == -1)
    {
        printf("Room not found.\n");
        return 0;
    }


    /* Check whether student already has a room */
    if (students[studentIndex].roomNo != 0)
    {
        printf("%s already has Room %d.\n", students[studentIndex].name,
               students[studentIndex].roomNo);

        return 0;
    }


    /* Check room capacity */
    if (rooms[roomIndex].occupied >=
        rooms[roomIndex].capacity)
    {
        printf("Room %d is full.\n", roomNo);

        return 0;
    }


    /*
       Save previous state before making the change.

       Currently previousRoomNo will normally be 0,
       because students without a room have roomNo = 0.
    */
    push_undo(
        studentIndex,
        students[studentIndex].roomNo
    );


    /* Allot room */
    students[studentIndex].roomNo = roomNo;

    /* Increase room occupancy */
    rooms[roomIndex].occupied++;

    printf("Room %d allotted to %s (%s). Beds taken: %d/%d.\n", roomNo,
           students[studentIndex].name, students[studentIndex].rollNo,
           rooms[roomIndex].occupied, rooms[roomIndex].capacity);

    return 1;
}


/*
   Undo the most recent room allotment.

   This demonstrates STACK LIFO behavior.

   Example:

   Allot 101
   Allot 102
   Allot 103

   Undo -> 103 is removed first.
*/
void undo_last_allotment(void)
{
    /* Check whether stack is empty */
    if (undoTop == -1)
    {
        printf("Nothing to undo.\n");
        return;
    }


    /* Get the top stack entry */
    AllotmentAction action = undoStack[undoTop];

    int studentIndex = action.studentIndex;


    /* Get student's current room */
    int currentRoom =
        students[studentIndex].roomNo;


    /* Find current room */
    int roomIndex =
        find_room(currentRoom);


    /* Decrease room occupancy */
    if (roomIndex != -1 &&
        rooms[roomIndex].occupied > 0)
    {
        rooms[roomIndex].occupied--;
    }


    /* Restore previous room */
    students[studentIndex].roomNo =
        action.previousRoomNo;


    /* Pop the stack */
    undoTop--;


    printf("Undone: Room %d allotment for %s.\n",
           currentRoom, students[studentIndex].rollNo);
}


/* ---- Stack applications: hostel accounts calculator ----
   The warden types a sum such as  48 * (100000 + 500 * 12)
   Three stack steps, as in the syllabus:
     1. Balanced bracket check     - array stack of brackets
     2. Infix -> postfix (Polish)  - array stack of operators
     3. Postfix evaluation         - LINKED LIST stack of numbers
*/

int brackets_balanced(const char* expr)
{
    char stack[MAX_EXPR];
    int top = -1;

    for (int i = 0; expr[i] != '\0'; i++)
    {
        char c = expr[i];

        if (c == '(' || c == '[' || c == '{')
        {
            stack[++top] = c;
        }
        else if (c == ')' || c == ']' || c == '}')
        {
            if (top == -1)
            {
                printf("Unbalanced: '%c' at position %d has no opening bracket.\n", c, i + 1);
                return 0;
            }

            char open = stack[top--];

            if ((open == '(' && c != ')') || (open == '[' && c != ']') ||
                (open == '{' && c != '}'))
            {
                printf("Unbalanced: '%c' is closed by '%c' at position %d.\n", open, c, i + 1);
                return 0;
            }
        }
    }

    if (top != -1)
    {
        printf("Unbalanced: '%c' is never closed.\n", stack[top]);
        return 0;
    }

    printf("Brackets balanced.\n");
    return 1;
}


typedef struct {
    int isNumber;
    long long value;
    char op;
} Token;


int precedence(char op)
{
    if (op == '*' || op == '/' || op == '%')
    {
        return 2;
    }

    if (op == '+' || op == '-')
    {
        return 1;
    }

    return 0;
}


/*
   Shunting-yard: numbers go straight to the output; an operator
   first pops every stacked operator of higher or equal precedence
   (left-to-right evaluation); ')' pops back to its '('.
   Brackets are already known to be balanced, so every opening
   bracket is stored as '('.

   Returns the number of output tokens, or -1 on a syntax error.
*/
int infix_to_postfix(const char* expr, Token out[])
{
    char opStack[MAX_EXPR];
    int top = -1;
    int n = 0;
    int expectNumber = 1;
    int i = 0;

    while (expr[i] != '\0')
    {
        char c = expr[i];

        if (c == ' ')
        {
            i++;
        }
        else if (isdigit((unsigned char)c))
        {
            if (!expectNumber)
            {
                printf("Missing operator before position %d.\n", i + 1);
                return -1;
            }

            long long value = 0;

            while (isdigit((unsigned char)expr[i]))
            {
                value = value * 10 + (expr[i] - '0');
                i++;
            }

            out[n].isNumber = 1;
            out[n].value = value;
            n++;
            expectNumber = 0;
        }
        else if (c == '(' || c == '[' || c == '{')
        {
            if (!expectNumber)
            {
                printf("Missing operator before position %d.\n", i + 1);
                return -1;
            }

            opStack[++top] = '(';
            i++;
        }
        else if (c == ')' || c == ']' || c == '}')
        {
            if (expectNumber)
            {
                printf("Missing number before position %d.\n", i + 1);
                return -1;
            }

            while (opStack[top] != '(')
            {
                out[n].isNumber = 0;
                out[n].op = opStack[top--];
                n++;
            }

            top--;      /* discard the '(' */
            i++;
        }
        else if (precedence(c) > 0)
        {
            if (expectNumber)
            {
                printf("Operator '%c' at position %d needs a number before it "
                       "(negative numbers are not supported).\n", c, i + 1);
                return -1;
            }

            while (top >= 0 && opStack[top] != '(' &&
                   precedence(opStack[top]) >= precedence(c))
            {
                out[n].isNumber = 0;
                out[n].op = opStack[top--];
                n++;
            }

            opStack[++top] = c;
            expectNumber = 1;
            i++;
        }
        else
        {
            printf("Invalid character '%c' at position %d.\n", c, i + 1);
            return -1;
        }
    }

    if (expectNumber)
    {
        printf("Expression is empty or ends with an operator.\n");
        return -1;
    }

    while (top >= 0)
    {
        out[n].isNumber = 0;
        out[n].op = opStack[top--];
        n++;
    }

    return n;
}


/* Stack as a linked list: push and pop at the head, O(1) */
typedef struct StackNode {
    long long value;
    struct StackNode* next;
} StackNode;


void ll_push(StackNode** top, long long value)
{
    StackNode* node = malloc(sizeof(StackNode));

    if (node == NULL)
    {
        printf("Out of memory.\n");
        return;
    }

    node->value = value;
    node->next = *top;
    *top = node;
}


long long ll_pop(StackNode** top)
{
    StackNode* node = *top;
    long long value = node->value;

    *top = node->next;
    free(node);

    return value;
}


/*
   Numbers are pushed; an operator pops two, applies itself and
   pushes the result. Returns 1 and sets *result on success.
*/
int evaluate_postfix(Token tokens[], int n, long long* result)
{
    StackNode* top = NULL;

    for (int i = 0; i < n; i++)
    {
        if (tokens[i].isNumber)
        {
            ll_push(&top, tokens[i].value);
            continue;
        }

        long long b = ll_pop(&top);
        long long a = ll_pop(&top);

        if ((tokens[i].op == '/' || tokens[i].op == '%') && b == 0)
        {
            printf("Cannot divide by zero.\n");

            while (top != NULL)
            {
                ll_pop(&top);
            }

            return 0;
        }

        switch (tokens[i].op)
        {
            case '+': ll_push(&top, a + b); break;
            case '-': ll_push(&top, a - b); break;
            case '*': ll_push(&top, a * b); break;
            case '/': ll_push(&top, a / b); break;
            default:  ll_push(&top, a % b); break;
        }
    }

    *result = ll_pop(&top);

    return 1;
}


void hostel_calculator(void)
{
    char expr[MAX_EXPR];
    Token tokens[MAX_EXPR];

    printf("Use numbers, + - * / %% and brackets ( ) [ ] { }\n");
    read_line("Expression (e.g. 48 * (100000 + 500 * 12)): ", expr, sizeof expr);

    if (!brackets_balanced(expr))
    {
        return;
    }

    int n = infix_to_postfix(expr, tokens);

    if (n == -1)
    {
        return;
    }

    printf("Postfix: ");

    for (int i = 0; i < n; i++)
    {
        if (tokens[i].isNumber)
        {
            printf("%lld ", tokens[i].value);
        }
        else
        {
            printf("%c ", tokens[i].op);
        }
    }

    printf("\n");

    long long result;

    if (evaluate_postfix(tokens, n, &result))
    {
        printf("Result: Rs %lld\n", result);
    }
}


/* ================================================================
   MODULE 3: QUEUES - WAITING LIST, COMPLAINTS, CLEANING RECORD
   ================================================================ */

/* ---- Circular queue: room allotment waiting list ---- */

int waitingList[MAX_STUDENTS];   /* student indices */
int wlFront = 0, wlRear = -1, wlCount = 0;


void enqueue_waiting(int studentIndex)
{
    if (wlCount >= MAX_STUDENTS)
    {
        printf("Waiting list is full.\n");
        return;
    }

    wlRear = (wlRear + 1) % MAX_STUDENTS;
    waitingList[wlRear] = studentIndex;
    wlCount++;

    printf("%s added to waiting list (position %d).\n",
           students[studentIndex].rollNo, wlCount);
}


/*
   Returns the student index at the front, or -1 if the queue is
   empty.
*/
int dequeue_waiting(void)
{
    if (wlCount == 0)
    {
        return -1;
    }

    int studentIndex = waitingList[wlFront];
    wlFront = (wlFront + 1) % MAX_STUDENTS;
    wlCount--;

    return studentIndex;
}


/*
   Walk the circular queue from front to rear.
*/
int is_in_waiting_list(int studentIndex)
{
    for (int i = 0; i < wlCount; i++)
    {
        if (waitingList[(wlFront + i) % MAX_STUDENTS] == studentIndex)
        {
            return 1;
        }
    }

    return 0;
}


/* ---- Priority queue: complaints by urgency (binary min-heap) ----
   A burst pipe (HIGH) is handled before a flickering bulb (LOW)
   even if the bulb was reported first. The heap is ordered by
   (priority, complaintId), so complaints of equal priority are
   handled in the order they were filed.

   Array form of a complete binary tree:
     parent(i) = (i - 1) / 2, children = 2i + 1 and 2i + 2
   Insert and remove are O(log n), versus O(n) shifting in a
   sorted array.
*/

Complaint complaintHeap[MAX_COMPLAINTS];
int complaintCount = 0;
int nextComplaintId = 1;


/*
   Does a belong ABOVE b in the heap?
   maxHeap = 0: more urgent on top (the complaint queue)
   maxHeap = 1: less urgent on top (used by heap sort)
*/
int complaint_above(const Complaint* a, const Complaint* b, int maxHeap)
{
    int moreUrgent = (a->priority != b->priority)
                         ? a->priority < b->priority
                         : a->complaintId < b->complaintId;

    return maxHeap ? !moreUrgent : moreUrgent;
}


void swap_complaints(Complaint* a, Complaint* b)
{
    Complaint t = *a;
    *a = *b;
    *b = t;
}


/* Heapify: push heap[i] down until both children are below it */
void heapify(Complaint heap[], int n, int i, int maxHeap)
{
    while (1)
    {
        int top = i;
        int left = 2 * i + 1;
        int right = 2 * i + 2;

        if (left < n && complaint_above(&heap[left], &heap[top], maxHeap))
        {
            top = left;
        }

        if (right < n && complaint_above(&heap[right], &heap[top], maxHeap))
        {
            top = right;
        }

        if (top == i)
        {
            return;
        }

        swap_complaints(&heap[i], &heap[top]);
        i = top;
    }
}


void complaint_enqueue(Complaint c)
{
    if (complaintCount >= MAX_COMPLAINTS)
    {
        printf("Complaint queue is full.\n");
        return;
    }

    /* Add at the end, then sift up past less urgent parents */
    int i = complaintCount++;
    complaintHeap[i] = c;

    while (i > 0 && complaint_above(&complaintHeap[i], &complaintHeap[(i - 1) / 2], 0))
    {
        swap_complaints(&complaintHeap[i], &complaintHeap[(i - 1) / 2]);
        i = (i - 1) / 2;
    }

    printf("Complaint #%d filed. %d complaint(s) pending.\n",
           c.complaintId, complaintCount);
}


/*
   Remove the root (most urgent). Move the last item to the root
   and heapify down. Caller must check complaintCount > 0 first.
*/
Complaint complaint_dequeue(void)
{
    Complaint c = complaintHeap[0];

    complaintHeap[0] = complaintHeap[--complaintCount];
    heapify(complaintHeap, complaintCount, 0, 0);

    return c;
}


/*
   Heap sort on a COPY of the queue, so the warden can see every
   pending complaint in the order they will be handled.
   Build a max-heap (least urgent on top), then repeatedly swap
   the top to the end of the array: the array ends up ordered
   most urgent first. O(n log n).
*/
void print_all_complaints(void)
{
    if (complaintCount == 0)
    {
        printf("No pending complaints.\n");
        return;
    }

    Complaint sorted[MAX_COMPLAINTS];
    int n = complaintCount;

    memcpy(sorted, complaintHeap, n * sizeof(Complaint));

    for (int i = n / 2 - 1; i >= 0; i--)
    {
        heapify(sorted, n, i, 1);
    }

    for (int end = n - 1; end > 0; end--)
    {
        swap_complaints(&sorted[0], &sorted[end]);
        heapify(sorted, end, 0, 1);
    }

    printf("Pending complaints, in the order they will be handled:\n");

    for (int i = 0; i < n; i++)
    {
        printf("  #%-3d %-6s Room %d (%s): %s\n", sorted[i].complaintId,
               priority_name(sorted[i].priority), sorted[i].roomNo,
               students[sorted[i].studentIndex].rollNo, sorted[i].description);
    }
}


const char* priority_name(Priority p)
{
    switch (p)
    {
        case COMP_HIGH:   return "HIGH";
        case COMP_MEDIUM: return "MEDIUM";
        default:          return "LOW";
    }
}


/* ---- Linked-list queue: daily room cleaning record ----
   Every day a student of the room confirms whether the cleaner
   came: sweeping only, or sweeping + mopping (due twice a week).
   Each room's record is a queue built from a linked list: new
   records join at the rear, and once CLEAN_HISTORY records are
   kept the oldest is dequeued from the front.
*/

typedef enum {
    CLEAN_NOT_DONE = 0,
    CLEAN_SWEEP = 1,
    CLEAN_SWEEP_MOP = 2
} CleanType;

typedef struct CleanNode {
    time_t date;
    CleanType type;
    int studentIndex;            /* who confirmed it */
    struct CleanNode* next;
} CleanNode;

CleanNode* cleanFront[MAX_ROOMS];
CleanNode* cleanRear[MAX_ROOMS];
int cleanCount[MAX_ROOMS];


const char* clean_name(CleanType t)
{
    switch (t)
    {
        case CLEAN_SWEEP:     return "Sweeping";
        case CLEAN_SWEEP_MOP: return "Sweeping + Mopping";
        default:              return "NOT cleaned";
    }
}


void clean_enqueue(int roomIndex, CleanNode* node)
{
    node->next = NULL;

    if (cleanRear[roomIndex] == NULL)
    {
        cleanFront[roomIndex] = node;    /* empty queue */
    }
    else
    {
        cleanRear[roomIndex]->next = node;
    }

    cleanRear[roomIndex] = node;
    cleanCount[roomIndex]++;

    if (cleanCount[roomIndex] > CLEAN_HISTORY)
    {
        /* Dequeue the oldest record */
        CleanNode* old = cleanFront[roomIndex];
        cleanFront[roomIndex] = old->next;
        free(old);
        cleanCount[roomIndex]--;
    }
}


void record_cleaning(int studentIndex)
{
    int roomIndex = find_room(students[studentIndex].roomNo);

    if (roomIndex == -1)
    {
        printf("%s has no room allotted.\n", students[studentIndex].name);
        return;
    }

    time_t now = time(NULL);

    if (cleanRear[roomIndex] != NULL && same_day(cleanRear[roomIndex]->date, now))
    {
        printf("Today's cleaning for Room %d is already recorded (%s).\n",
               rooms[roomIndex].roomNo, clean_name(cleanRear[roomIndex]->type));
        return;
    }

    int came = read_int_range("Did the cleaner come and clean your room today? (1 = Yes, 2 = No): ", 1, 2);

    CleanType type = CLEAN_NOT_DONE;

    if (came == 1)
    {
        type = (CleanType)read_int_range("What was done? (1 = Sweeping only, 2 = Sweeping + Mopping): ", 1, 2);
    }

    CleanNode* node = malloc(sizeof(CleanNode));

    if (node == NULL)
    {
        printf("Out of memory.\n");
        return;
    }

    node->date = now;
    node->type = type;
    node->studentIndex = studentIndex;

    clean_enqueue(roomIndex, node);

    printf("Recorded for Room %d: %s.\n", rooms[roomIndex].roomNo, clean_name(type));
}


/* Walk the queue front to rear, then summarise the last 7 days */
void print_cleaning_record(int roomNo)
{
    int roomIndex = find_room(roomNo);

    if (roomIndex == -1)
    {
        printf("Room not found.\n");
        return;
    }

    if (cleanFront[roomIndex] == NULL)
    {
        printf("No cleaning records for Room %d.\n", roomNo);
        return;
    }

    printf("Cleaning record for Room %d (oldest first):\n", roomNo);

    time_t now = time(NULL);
    int swept = 0, mopped = 0, missed = 0;
    char when[32];

    for (CleanNode* p = cleanFront[roomIndex]; p != NULL; p = p->next)
    {
        strftime(when, sizeof when, "%a %d-%m-%Y", localtime(&p->date));
        printf("  %s  %-20s (confirmed by %s)\n", when, clean_name(p->type),
               students[p->studentIndex].rollNo);

        if (difftime(now, p->date) < 7 * 24 * 60 * 60)
        {
            if (p->type == CLEAN_SWEEP)          swept++;
            else if (p->type == CLEAN_SWEEP_MOP) mopped++;
            else                                 missed++;
        }
    }

    printf("Last 7 days: %d sweeping only, %d sweeping + mopping, %d not cleaned.\n",
           swept, mopped, missed);

    if (mopped < 2)
    {
        printf("Note: mopping is due twice a week (%d done).\n", mopped);
    }
}


/* ================================================================
   MODULE 4: LINKED LISTS - GATE LOG, STUDENTS OUTSIDE, DUTY ROSTER
   ================================================================ */

/* ---- Doubly linked list: per-student gate entry/exit history ----
   Doubly linked so the warden can walk backwards from the most
   recent entry - "when did this student last leave?"
*/

typedef struct GateNode {
    char type[4];        /* "IN" or "OUT" */
    time_t timestamp;
    struct GateNode* prev;
    struct GateNode* next;
} GateNode;

GateNode* gateHead[MAX_STUDENTS];
GateNode* gateTail[MAX_STUDENTS];


void log_gate_entry(int studentIndex, const char* type, time_t timestamp)
{
    GateNode* node = malloc(sizeof(GateNode));

    if (node == NULL)
    {
        printf("Out of memory.\n");
        return;
    }

    strncpy(node->type, type, sizeof node->type - 1);
    node->type[sizeof node->type - 1] = '\0';
    node->timestamp = timestamp;
    node->next = NULL;
    node->prev = gateTail[studentIndex];

    if (gateHead[studentIndex] == NULL)
    {
        /* Empty list: node is both head and tail */
        gateHead[studentIndex] = node;
    }
    else
    {
        gateTail[studentIndex]->next = node;
    }

    gateTail[studentIndex] = node;
}


void print_gate_log_recent_first(int studentIndex)
{
    if (gateTail[studentIndex] == NULL)
    {
        printf("No gate entries for %s.\n", students[studentIndex].name);
        return;
    }

    printf("Gate log for %s (%s), most recent first:\n",
           students[studentIndex].name, students[studentIndex].rollNo);

    char when[32];

    for (GateNode* p = gateTail[studentIndex]; p != NULL; p = p->prev)
    {
        format_time(p->timestamp, when, sizeof when);
        printf("  %-4s %s\n", p->type, when);
    }
}


/* ---- Doubly linked list: students currently outside campus ----
   The warden's office list. Going OUT appends a node at the tail.
   Coming IN removes that student's node - outNodeOf[] remembers
   where it is, and prev/next pointers let us unlink it in O(1)
   without searching the list.
*/

typedef struct OutNode {
    int studentIndex;
    time_t outTime;
    struct OutNode* prev;
    struct OutNode* next;
} OutNode;

OutNode* outHead = NULL;
OutNode* outTail = NULL;
OutNode* outNodeOf[MAX_STUDENTS];
int outsideCount = 0;


int is_outside(int studentIndex)
{
    return outNodeOf[studentIndex] != NULL;
}


/* Gate is closed from curfew until GATE_OPEN_HOUR in the morning */
int gate_closed(void)
{
    time_t now = time(NULL);
    struct tm* t = localtime(&now);

    return t->tm_hour >= curfewHour || t->tm_hour < GATE_OPEN_HOUR;
}


void going_out(int studentIndex)
{
    Student* s = &students[studentIndex];

    if (is_outside(studentIndex))
    {
        printf("%s is already outside campus.\n", s->name);
        return;
    }

    if (gate_closed())
    {
        printf("Gate is closed (curfew %d PM - %d AM). Cannot go out.\n",
               curfewHour - 12, GATE_OPEN_HOUR);
        return;
    }

    OutNode* node = malloc(sizeof(OutNode));

    if (node == NULL)
    {
        printf("Out of memory.\n");
        return;
    }

    time_t now = time(NULL);

    node->studentIndex = studentIndex;
    node->outTime = now;
    node->next = NULL;
    node->prev = outTail;

    if (outHead == NULL)
    {
        outHead = node;
    }
    else
    {
        outTail->next = node;
    }

    outTail = node;
    outNodeOf[studentIndex] = node;
    outsideCount++;

    log_gate_entry(studentIndex, "OUT", now);

    char when[32];
    format_time(now, when, sizeof when);

    printf("Recorded at warden's office - going OUT at %s:\n", when);
    printf("  %s (%s), phone %s, parents %s / %s\n", s->name, s->rollNo,
           s->phone, s->parentPhone1, s->parentPhone2);
    printf("  Must be back before %d PM.\n", curfewHour - 12);
}


void coming_in(int studentIndex)
{
    OutNode* node = outNodeOf[studentIndex];

    if (node == NULL)
    {
        printf("%s is not marked as outside.\n", students[studentIndex].name);
        return;
    }

    if (gate_closed())
    {
        printf("No entry after %d PM. Gate opens at %d AM.\n",
               curfewHour - 12, GATE_OPEN_HOUR);
        return;
    }

    /* Unlink in O(1) using prev/next */
    if (node->prev == NULL)
    {
        outHead = node->next;
    }
    else
    {
        node->prev->next = node->next;
    }

    if (node->next == NULL)
    {
        outTail = node->prev;
    }
    else
    {
        node->next->prev = node->prev;
    }

    free(node);
    outNodeOf[studentIndex] = NULL;
    outsideCount--;

    log_gate_entry(studentIndex, "IN", time(NULL));

    printf("%s (%s) is back in the hostel.\n", students[studentIndex].name,
           students[studentIndex].rollNo);
}


/* Warden's office view: everyone outside, earliest to leave first */
void print_students_outside(void)
{
    if (outHead == NULL)
    {
        printf("All students are in the hostel.\n");
        return;
    }

    printf("%d student(s) outside campus:\n", outsideCount);
    printf("%-6s %-20s %-5s %-11s %-11s %-11s %s\n", "Roll", "Name", "Room",
           "Phone", "Parent 1", "Parent 2", "Went out");

    char when[32];

    for (OutNode* p = outHead; p != NULL; p = p->next)
    {
        Student* s = &students[p->studentIndex];
        format_time(p->outTime, when, sizeof when);

        printf("%-6s %-20s %-5d %-11s %-11s %-11s %s\n", s->rollNo, s->name,
               s->roomNo, s->phone, s->parentPhone1, s->parentPhone2, when);
    }
}


/* ---- Singly linked list: staff duty roster per floor ----
   Floors gain/lose staff dynamically, so a linked list beats a
   fixed array here.
*/

typedef struct DutyNode {
    int staffId;
    struct DutyNode* next;
} DutyNode;

DutyNode* floorDuty[NUM_FLOORS + 1];    /* floorDuty[f] = staff on floor f */


const char* role_name(StaffRole r)
{
    switch (r)
    {
        case ROLE_SWEEPER:     return "Sweeper";
        case ROLE_ELECTRICIAN: return "Electrician";
        case ROLE_PLUMBER:     return "Plumber";
        default:               return "Security";
    }
}


void add_staff(int staffId, const char* name, StaffRole role)
{
    if (staffCount >= MAX_STAFF)
    {
        return;
    }

    staff[staffCount].staffId = staffId;
    strncpy(staff[staffCount].name, name, MAX_NAME - 1);
    staff[staffCount].name[MAX_NAME - 1] = '\0';
    staff[staffCount].role = role;
    staff[staffCount].assignedFloor = 0;
    staffCount++;
}


void initialize_staff(void)
{
    add_staff(1, "Ramesh", ROLE_SWEEPER);
    add_staff(2, "Suresh", ROLE_SWEEPER);
    add_staff(3, "Mahesh", ROLE_ELECTRICIAN);
    add_staff(4, "Dinesh", ROLE_PLUMBER);
    add_staff(5, "Prakash", ROLE_SECURITY);
    add_staff(6, "Ganesh", ROLE_SECURITY);
}


int find_staff(int staffId)
{
    for (int i = 0; i < staffCount; i++)
    {
        if (staff[i].staffId == staffId)
        {
            return i;
        }
    }

    return -1;
}


void print_staff_list(void)
{
    printf("ID  Name        Role         Floor\n");

    for (int i = 0; i < staffCount; i++)
    {
        printf("%-3d %-11s %-12s ", staff[i].staffId, staff[i].name,
               role_name(staff[i].role));

        if (staff[i].assignedFloor == 0)
        {
            printf("-\n");
        }
        else
        {
            printf("%d\n", staff[i].assignedFloor);
        }
    }
}


/*
   Prepend to floorDuty[floor] - O(1) insert, no list walk.
   A staff member is on one floor at a time, so assigning them
   elsewhere first removes them from their old floor.
*/
void assign_staff_to_floor(int floor, int staffId)
{
    int s = find_staff(staffId);

    if (s == -1)
    {
        printf("Staff ID %d not found.\n", staffId);
        return;
    }

    if (staff[s].assignedFloor == floor)
    {
        printf("%s is already on floor %d.\n", staff[s].name, floor);
        return;
    }

    if (staff[s].assignedFloor != 0)
    {
        remove_staff_from_floor(staff[s].assignedFloor, staffId);
    }

    DutyNode* node = malloc(sizeof(DutyNode));

    if (node == NULL)
    {
        printf("Out of memory.\n");
        return;
    }

    node->staffId = staffId;
    node->next = floorDuty[floor];
    floorDuty[floor] = node;

    staff[s].assignedFloor = floor;

    printf("%s (%s) assigned to floor %d.\n", staff[s].name,
           role_name(staff[s].role), floor);
}


/*
   Unlink and free the node with matching staffId.
   Head node is a special case: floorDuty[floor] itself changes.
*/
void remove_staff_from_floor(int floor, int staffId)
{
    DutyNode* curr = floorDuty[floor];
    DutyNode* prev = NULL;

    while (curr != NULL && curr->staffId != staffId)
    {
        prev = curr;
        curr = curr->next;
    }

    if (curr == NULL)
    {
        printf("Staff ID %d is not on floor %d.\n", staffId, floor);
        return;
    }

    if (prev == NULL)
    {
        floorDuty[floor] = curr->next;      /* deleting the head */
    }
    else
    {
        prev->next = curr->next;
    }

    free(curr);

    int s = find_staff(staffId);

    if (s != -1)
    {
        staff[s].assignedFloor = 0;
        printf("%s removed from floor %d.\n", staff[s].name, floor);
    }
}


void print_floor_duty(int floor)
{
    if (floorDuty[floor] == NULL)
    {
        printf("No staff on duty on floor %d.\n", floor);
        return;
    }

    printf("Staff on duty on floor %d:\n", floor);

    for (DutyNode* p = floorDuty[floor]; p != NULL; p = p->next)
    {
        int s = find_staff(p->staffId);
        printf("  [%d] %s - %s\n", p->staffId, staff[s].name,
               role_name(staff[s].role));
    }
}


/* ================================================================
   MODULE 5: HASHING - CHAINING (ROLL NO) + OPEN ADDRESSING (PHONE)
   ================================================================
   Map roll number -> index in students[]. Chaining handles
   collisions.

   String hash: treat the roll number as a number in base 31,
   reduced mod HASH_SIZE at each step (Horner's method).
   HASH_SIZE is the prime 101 so the letters and digits all
   affect the bucket; a round size like 100 would effectively
   look only at the last couple of characters.
*/

typedef struct HashNode {
    char rollNo[ROLL_LEN];
    int studentIndex;
    struct HashNode* next;    /* chaining */
} HashNode;

HashNode* hashTable[HASH_SIZE];


int hash_function(const char* rollNo)
{
    unsigned int h = 0;

    for (int i = 0; rollNo[i] != '\0'; i++)
    {
        h = (h * 31 + (unsigned char)rollNo[i]) % HASH_SIZE;
    }

    return (int)h;
}


/* Insert at the HEAD of the bucket's chain - O(1) */
void hash_insert(const char* rollNo, int studentIndex)
{
    HashNode* node = malloc(sizeof(HashNode));

    if (node == NULL)
    {
        printf("Out of memory.\n");
        return;
    }

    int bucket = hash_function(rollNo);

    strcpy(node->rollNo, rollNo);
    node->studentIndex = studentIndex;
    node->next = hashTable[bucket];
    hashTable[bucket] = node;
}


/* Returns studentIndex, or -1 if absent */
int hash_lookup(const char* rollNo)
{
    for (HashNode* p = hashTable[hash_function(rollNo)]; p != NULL; p = p->next)
    {
        if (strcmp(p->rollNo, rollNo) == 0)
        {
            return p->studentIndex;
        }
    }

    return -1;
}


/* ---- Open addressing: who does this phone number belong to? ----
   When a call comes to the warden's office, look up the number
   among every student's phone and both parent phones.

   Collisions are handled by LINEAR PROBING instead of chaining:
   if a slot is taken, try the next one (wrapping around) until an
   empty slot is found. No linked lists - everything lives in one
   array. Siblings can share a parent's number, so a lookup keeps
   probing until it hits an empty slot and reports every match.
*/

typedef struct {
    int used;
    char phone[PHONE_LEN];
    int studentIndex;
    int whose;           /* 0 = student, 1 = parent 1, 2 = parent 2 */
} PhoneSlot;

PhoneSlot phoneTable[PHONE_TABLE_SIZE];


/* 10-digit number mod table size, digit by digit (Horner) */
int phone_hash(const char* phone)
{
    long long h = 0;

    for (int i = 0; phone[i] != '\0'; i++)
    {
        h = (h * 10 + (phone[i] - '0')) % PHONE_TABLE_SIZE;
    }

    return (int)h;
}


void phone_insert(const char* phone, int studentIndex, int whose)
{
    int h = phone_hash(phone);

    for (int i = 0; i < PHONE_TABLE_SIZE; i++)
    {
        int slot = (h + i) % PHONE_TABLE_SIZE;

        if (!phoneTable[slot].used)
        {
            phoneTable[slot].used = 1;
            strcpy(phoneTable[slot].phone, phone);
            phoneTable[slot].studentIndex = studentIndex;
            phoneTable[slot].whose = whose;
            return;
        }
    }

    printf("Phone table is full.\n");
}


void find_by_phone(const char* phone)
{
    const char* whoseName[] = { "Student", "Parent 1 of", "Parent 2 of" };

    int h = phone_hash(phone);
    int found = 0;
    int probes = 0;

    for (int i = 0; i < PHONE_TABLE_SIZE; i++)
    {
        int slot = (h + i) % PHONE_TABLE_SIZE;
        probes++;

        if (!phoneTable[slot].used)
        {
            break;      /* empty slot: number can't be further on */
        }

        if (strcmp(phoneTable[slot].phone, phone) == 0)
        {
            Student* s = &students[phoneTable[slot].studentIndex];

            printf("  %s %s (%s)", whoseName[phoneTable[slot].whose],
                   s->name, s->rollNo);

            if (s->roomNo != 0)
            {
                printf(", Room %d", s->roomNo);
            }

            printf("\n");
            found++;
        }
    }

    if (found == 0)
    {
        printf("Number %s is not registered.\n", phone);
    }

    printf("(home slot %d, %d slot(s) probed)\n", h, probes);
}


/* ================================================================
   MODULE 6: AVL TREE - HOSTEL FEE PAYMENTS
   ================================================================
   Hostel fee is Rs 1,00,000 per academic year, due in July. When a
   student pays, they are inserted into a tree keyed on payment
   time. In-order traversal lists payments in the order they were
   made, with no sorting step. Students not in the tree are the
   ones still pending.

   Why AVL and not a plain BST: payments arrive in time order, so
   every new key is the largest so far. A plain BST would put each
   one to the right of the last - a "tree" of height n that is
   really a linked list. AVL rotations keep the height ~log2(n).

   Cancelling a payment (e.g. cheque bounced) is a BST deletion,
   followed by the same rebalancing.
*/

struct AVLNode {
    int studentIndex;
    int height;              /* leaf = 1 */
    struct AVLNode* left;
    struct AVLNode* right;
};

AVLNode* feeTreeRoot = NULL;
int feeTreeSize = 0;


/*
   Order by payment time; ties broken by student index so every
   key is unique and deletion finds exactly one node.
*/
int fee_key_compare(int a, int b)
{
    if (students[a].feePaidOn != students[b].feePaidOn)
    {
        return students[a].feePaidOn < students[b].feePaidOn ? -1 : 1;
    }

    return (a > b) - (a < b);
}


int avl_height(AVLNode* n)
{
    return n == NULL ? 0 : n->height;
}


void avl_update_height(AVLNode* n)
{
    int hl = avl_height(n->left);
    int hr = avl_height(n->right);

    n->height = 1 + (hl > hr ? hl : hr);
}


int avl_balance(AVLNode* n)
{
    return n == NULL ? 0 : avl_height(n->left) - avl_height(n->right);
}


/*
        y                x
       / \              / \
      x   C    -->     A   y
     / \                  / \
    A   B                B   C
*/
AVLNode* rotate_right(AVLNode* y)
{
    AVLNode* x = y->left;

    y->left = x->right;
    x->right = y;

    avl_update_height(y);
    avl_update_height(x);

    return x;
}


/* Mirror image of rotate_right */
AVLNode* rotate_left(AVLNode* x)
{
    AVLNode* y = x->right;

    x->right = y->left;
    y->left = x;

    avl_update_height(x);
    avl_update_height(y);

    return y;
}


/*
   After an insert or delete below this node, fix it if one side
   is more than 1 taller than the other. The four cases:
     LL -> rotate right          RR -> rotate left
     LR -> rotate left at child, then right
     RL -> rotate right at child, then left
*/
AVLNode* avl_rebalance(AVLNode* node)
{
    avl_update_height(node);

    int balance = avl_balance(node);

    if (balance > 1)
    {
        if (avl_balance(node->left) < 0)
        {
            printf("  (AVL: LR rotation)\n");
            node->left = rotate_left(node->left);
        }
        else
        {
            printf("  (AVL: LL rotation)\n");
        }

        return rotate_right(node);
    }

    if (balance < -1)
    {
        if (avl_balance(node->right) > 0)
        {
            printf("  (AVL: RL rotation)\n");
            node->right = rotate_right(node->right);
        }
        else
        {
            printf("  (AVL: RR rotation)\n");
        }

        return rotate_left(node);
    }

    return node;
}


AVLNode* avl_insert(AVLNode* root, int studentIndex)
{
    if (root == NULL)
    {
        AVLNode* node = malloc(sizeof(AVLNode));

        if (node == NULL)
        {
            printf("Out of memory.\n");
            return NULL;
        }

        node->studentIndex = studentIndex;
        node->height = 1;
        node->left = NULL;
        node->right = NULL;
        feeTreeSize++;
        return node;
    }

    if (fee_key_compare(studentIndex, root->studentIndex) < 0)
    {
        root->left = avl_insert(root->left, studentIndex);
    }
    else
    {
        root->right = avl_insert(root->right, studentIndex);
    }

    return avl_rebalance(root);
}


/*
   BST deletion, three cases:
     no child  -> just remove it
     one child -> replace the node with that child
     two kids  -> copy in the in-order successor (smallest in the
                  right subtree), then delete the successor
   Rebalance on the way back up.
*/
AVLNode* avl_delete(AVLNode* root, int studentIndex)
{
    if (root == NULL)
    {
        return NULL;
    }

    int cmp = fee_key_compare(studentIndex, root->studentIndex);

    if (cmp < 0)
    {
        root->left = avl_delete(root->left, studentIndex);
    }
    else if (cmp > 0)
    {
        root->right = avl_delete(root->right, studentIndex);
    }
    else if (root->left == NULL || root->right == NULL)
    {
        AVLNode* child = root->left != NULL ? root->left : root->right;
        free(root);
        feeTreeSize--;
        return child;
    }
    else
    {
        AVLNode* succ = root->right;

        while (succ->left != NULL)
        {
            succ = succ->left;
        }

        root->studentIndex = succ->studentIndex;
        root->right = avl_delete(root->right, succ->studentIndex);
    }

    return avl_rebalance(root);
}


/* In-order: left, root, right = earliest payment first */
void avl_inorder(AVLNode* root)
{
    if (root == NULL)
    {
        return;
    }

    avl_inorder(root->left);

    Student* s = &students[root->studentIndex];
    char when[32];
    format_time(s->feePaidOn, when, sizeof when);
    printf("  %s  %-6s %s\n", when, s->rollNo, s->name);

    avl_inorder(root->right);
}


void print_fee_payments(void)
{
    if (feeTreeRoot == NULL)
    {
        printf("No fee payments yet.\n");
        return;
    }

    printf("Hostel fee payments (earliest first):\n");
    avl_inorder(feeTreeRoot);

    printf("%d payment(s), Rs %ld collected. Tree height: %d "
           "(a plain BST would have height %d).\n",
           feeTreeSize, (long)feeTreeSize * HOSTEL_FEE,
           avl_height(feeTreeRoot), feeTreeSize);
}


void cancel_fee_payment(int studentIndex)
{
    Student* s = &students[studentIndex];

    if (!s->hostelFeePaid)
    {
        printf("%s has not paid, nothing to cancel.\n", s->name);
        return;
    }

    /* Delete while feePaidOn still holds the key */
    feeTreeRoot = avl_delete(feeTreeRoot, studentIndex);

    s->hostelFeePaid = 0;
    s->feePaidOn = 0;

    printf("Fee payment cancelled for %s (%s). Fee is pending again.\n",
           s->name, s->rollNo);
}


void pay_hostel_fee(int studentIndex)
{
    Student* s = &students[studentIndex];

    if (s->hostelFeePaid)
    {
        printf("%s has already paid the hostel fee.\n", s->name);
        return;
    }

    printf("Hostel fee for the academic year: Rs %d (due in July).\n", HOSTEL_FEE);

    if (read_int_range("Pay now? (1 = Yes, 2 = No): ", 1, 2) != 1)
    {
        printf("Fee still pending.\n");
        return;
    }

    s->hostelFeePaid = 1;
    s->feePaidOn = time(NULL);
    feeTreeRoot = avl_insert(feeTreeRoot, studentIndex);

    printf("Hostel fee paid for %s (%s).\n", s->name, s->rollNo);
}


void print_pending_fees(void)
{
    int pending = 0;

    for (int i = 0; i < studentCount; i++)
    {
        if (!students[i].hostelFeePaid)
        {
            printf("  %-6s %-20s phone %s\n", students[i].rollNo,
                   students[i].name, students[i].phone);
            pending++;
        }
    }

    if (pending == 0)
    {
        printf("No pending hostel fees.\n");
    }
    else
    {
        printf("%d student(s) pending, Rs %ld outstanding.\n", pending,
               (long)pending * HOSTEL_FEE);
    }
}


/* ---- Laundry: Rs 500 per month for 75 clothes ----
   laundryMonth stores the YYYYMM the student last paid for, so
   comparing it with current_month() resets everyone
   automatically when a new month starts.
*/

void pay_laundry(int studentIndex)
{
    Student* s = &students[studentIndex];

    if (s->laundryMonth == current_month())
    {
        printf("%s already has laundry for this month.\n", s->name);
        return;
    }

    printf("Laundry: Rs %d for %d clothes this month.\n", LAUNDRY_FEE, LAUNDRY_CLOTHES);

    if (read_int_range("Do you want to pay? (1 = Yes, 2 = No): ", 1, 2) == 1)
    {
        s->laundryMonth = current_month();
        printf("Laundry active for %s this month.\n", s->name);
    }
    else
    {
        printf("Laundry not taken this month.\n");
    }
}


void print_laundry_status(void)
{
    int month = current_month();
    int active = 0;

    printf("Laundry status for %02d-%d:\n", month % 100, month / 100);

    for (int i = 0; i < studentCount; i++)
    {
        int paid = (students[i].laundryMonth == month);
        printf("  %-6s %-20s %s\n", students[i].rollNo, students[i].name,
               paid ? "paid" : "not taken");
        active += paid;
    }

    printf("%d of %d students, Rs %d collected.\n", active, studentCount,
           active * LAUNDRY_FEE);
}


/* ---- Huffman coding: compressing a warden notice ----
   Notices go out to every student, so shorter is cheaper to send.
   Huffman gives frequent letters short bit codes and rare letters
   long ones:
     1. Count how often each character appears.
     2. Put one leaf per character in a MIN-HEAP by frequency.
     3. Repeatedly remove the two rarest, join them under a new
        node (frequency = sum), and put that back.
     4. The last node is the root. Left edge = 0, right edge = 1;
        a character's code is the path from root to its leaf.
*/

typedef struct HuffNode {
    unsigned char ch;
    int freq;
    struct HuffNode* left;
    struct HuffNode* right;
} HuffNode;

HuffNode* huffHeap[256];
int huffHeapSize = 0;


int huff_before(HuffNode* a, HuffNode* b)
{
    if (a->freq != b->freq)
    {
        return a->freq < b->freq;
    }

    return a->ch < b->ch;       /* tie-break so output is repeatable */
}


void huff_push(HuffNode* node)
{
    int i = huffHeapSize++;
    huffHeap[i] = node;

    while (i > 0 && huff_before(huffHeap[i], huffHeap[(i - 1) / 2]))
    {
        HuffNode* t = huffHeap[i];
        huffHeap[i] = huffHeap[(i - 1) / 2];
        huffHeap[(i - 1) / 2] = t;
        i = (i - 1) / 2;
    }
}


HuffNode* huff_pop(void)
{
    HuffNode* min = huffHeap[0];
    huffHeap[0] = huffHeap[--huffHeapSize];

    int i = 0;

    while (1)
    {
        int small = i;
        int left = 2 * i + 1;
        int right = 2 * i + 2;

        if (left < huffHeapSize && huff_before(huffHeap[left], huffHeap[small]))
        {
            small = left;
        }

        if (right < huffHeapSize && huff_before(huffHeap[right], huffHeap[small]))
        {
            small = right;
        }

        if (small == i)
        {
            break;
        }

        HuffNode* t = huffHeap[i];
        huffHeap[i] = huffHeap[small];
        huffHeap[small] = t;
        i = small;
    }

    return min;
}


HuffNode* new_huff_node(unsigned char ch, int freq, HuffNode* left, HuffNode* right)
{
    HuffNode* node = malloc(sizeof(HuffNode));

    if (node == NULL)
    {
        printf("Out of memory.\n");
        exit(1);
    }

    node->ch = ch;
    node->freq = freq;
    node->left = left;
    node->right = right;

    return node;
}


char huffCodes[256][MAX_NOTICE + 1];


/* Walk the tree; at each leaf, the path so far is its code */
void huff_assign_codes(HuffNode* node, char* path, int depth)
{
    if (node->left == NULL && node->right == NULL)
    {
        path[depth] = '\0';
        strcpy(huffCodes[node->ch], depth == 0 ? "0" : path);
        return;
    }

    path[depth] = '0';
    huff_assign_codes(node->left, path, depth + 1);

    path[depth] = '1';
    huff_assign_codes(node->right, path, depth + 1);
}


void free_huff(HuffNode* node)
{
    if (node == NULL)
    {
        return;
    }

    free_huff(node->left);
    free_huff(node->right);
    free(node);
}


void compress_notice(void)
{
    char notice[MAX_NOTICE];

    read_line("Type the notice: ", notice, sizeof notice);

    int len = (int)strlen(notice);

    if (len == 0)
    {
        printf("Notice is empty.\n");
        return;
    }

    int freq[256] = { 0 };

    for (int i = 0; i < len; i++)
    {
        freq[(unsigned char)notice[i]]++;
    }

    huffHeapSize = 0;

    for (int c = 0; c < 256; c++)
    {
        if (freq[c] > 0)
        {
            huff_push(new_huff_node((unsigned char)c, freq[c], NULL, NULL));
        }
    }

    while (huffHeapSize > 1)
    {
        HuffNode* a = huff_pop();
        HuffNode* b = huff_pop();
        huff_push(new_huff_node(a->ch < b->ch ? a->ch : b->ch,
                                a->freq + b->freq, a, b));
    }

    HuffNode* root = huff_pop();
    char path[MAX_NOTICE + 1];

    huff_assign_codes(root, path, 0);

    printf("Char    Count  Code\n");

    long compressedBits = 0;

    for (int c = 0; c < 256; c++)
    {
        if (freq[c] > 0)
        {
            if (c == ' ')
            {
                printf("space   ");
            }
            else
            {
                printf("'%c'     ", c);
            }

            printf("%-6d %s\n", freq[c], huffCodes[c]);
            compressedBits += (long)freq[c] * (long)strlen(huffCodes[c]);
        }
    }

    printf("Encoded: ");

    int shown = 0;

    for (int i = 0; i < len && shown < 96; i++)
    {
        printf("%s", huffCodes[(unsigned char)notice[i]]);
        shown += (int)strlen(huffCodes[(unsigned char)notice[i]]);
    }

    printf("%s\n", shown < compressedBits ? "..." : "");

    printf("Original: %d bits (8 per character). Huffman: %ld bits. Saved %.1f%%.\n",
           len * 8, compressedBits, 100.0 * (len * 8 - compressedBits) / (len * 8));

    free_huff(root);
}


/* ================================================================
   MODULE 7: GRAPH - FIRE EXIT ROUTES, CABLE LAYOUT (MST)
   ================================================================
   Undirected weighted graph:
     nodes    = rooms, corridor points, staircases, exits
     edges    = physical connections
     weights  = walking distance in metres

   Every floor is one straight corridor with three staircases
   (A, B, C) that go down to three ground-floor exits:

   Stair A -5- [101] -8- [102] ---20--- Stair B ---20--- [103] -8- [104] -5- Stair C
      |                                    |                                  |
   (10 m per floor down to the ground floor, then 5 m to Exit A / B / C)

   [x0y] is the corridor point outside each room's door (2 m from
   the room). Stair B is set back from the rooms (20 m away).

   From Room 302, BFS picks Stair B (fewest points passed) but
   Dijkstra picks Stair A (fewest metres): the route through
   fewer junctions is not always the shortest walk.

   NOTE: distances are placeholders - measure your real hostel.
*/

typedef struct AdjNode {
    int nodeIndex;
    int distance;          /* edge weight in metres */
    struct AdjNode* next;
} AdjNode;

AdjNode* floorGraph[MAX_NODES];
char nodeNames[MAX_NODES][MAX_NAME];   /* "Room 101", "F1 Stair A", "Exit A" */
int isExit[MAX_NODES];                 /* 1 if this node is an exit */
int nodeRoomNo[MAX_NODES];             /* room number, 0 if not a room */
int graphNodeCount = 0;


int add_graph_node(const char* name, int exitFlag, int roomNo)
{
    int n = graphNodeCount++;

    strncpy(nodeNames[n], name, MAX_NAME - 1);
    nodeNames[n][MAX_NAME - 1] = '\0';
    isExit[n] = exitFlag;
    nodeRoomNo[n] = roomNo;
    floorGraph[n] = NULL;

    return n;
}


void add_edge(int from, int to, int distance)
{
    AdjNode* node = malloc(sizeof(AdjNode));

    if (node == NULL)
    {
        printf("Out of memory.\n");
        return;
    }

    node->nodeIndex = to;
    node->distance = distance;
    node->next = floorGraph[from];
    floorGraph[from] = node;
}


/* Undirected: add to BOTH lists */
void add_corridor(int a, int b, int distance)
{
    add_edge(a, b, distance);
    add_edge(b, a, distance);
}


/*
   Unlink the edge from -> to. Returns its distance, or -1 if
   there is no such edge.
*/
int remove_edge(int from, int to)
{
    AdjNode* curr = floorGraph[from];
    AdjNode* prev = NULL;

    while (curr != NULL && curr->nodeIndex != to)
    {
        prev = curr;
        curr = curr->next;
    }

    if (curr == NULL)
    {
        return -1;
    }

    if (prev == NULL)
    {
        floorGraph[from] = curr->next;
    }
    else
    {
        prev->next = curr->next;
    }

    int distance = curr->distance;
    free(curr);

    return distance;
}


int edge_distance(int a, int b)
{
    for (AdjNode* p = floorGraph[a]; p != NULL; p = p->next)
    {
        if (p->nodeIndex == b)
        {
            return p->distance;
        }
    }

    return -1;
}


void build_floor_graph(void)
{
    char name[MAX_NAME];
    const char stairLetter[3] = { 'A', 'B', 'C' };
    int exitNode[3];
    int stairBelow[3];

    for (int s = 0; s < 3; s++)
    {
        snprintf(name, sizeof name, "Exit %c", stairLetter[s]);
        exitNode[s] = add_graph_node(name, 1, 0);
        stairBelow[s] = exitNode[s];
    }

    for (int f = 1; f <= NUM_FLOORS; f++)
    {
        int stair[3];

        for (int s = 0; s < 3; s++)
        {
            snprintf(name, sizeof name, "F%d Stair %c", f, stairLetter[s]);
            stair[s] = add_graph_node(name, 0, 0);

            /* Down one floor, or to the exit from floor 1 */
            add_corridor(stair[s], stairBelow[s], f == 1 ? 5 : 10);
            stairBelow[s] = stair[s];
        }

        int outside[ROOMS_PER_FLOOR];

        for (int r = 1; r <= ROOMS_PER_FLOOR; r++)
        {
            int roomNo = f * 100 + r;

            snprintf(name, sizeof name, "Outside %d", roomNo);
            outside[r - 1] = add_graph_node(name, 0, 0);

            snprintf(name, sizeof name, "Room %d", roomNo);
            int room = add_graph_node(name, 0, roomNo);

            add_corridor(room, outside[r - 1], 2);
        }

        /* The straight corridor, west to east */
        add_corridor(stair[0], outside[0], 5);
        add_corridor(outside[0], outside[1], 8);
        add_corridor(outside[1], stair[1], 20);
        add_corridor(stair[1], outside[2], 20);
        add_corridor(outside[2], outside[3], 8);
        add_corridor(outside[3], stair[2], 5);
    }
}


int room_to_node(int roomNo)
{
    for (int i = 0; i < graphNodeCount; i++)
    {
        if (nodeRoomNo[i] == roomNo && roomNo != 0)
        {
            return i;
        }
    }

    return -1;
}


/*
   Print start -> ... -> end using parent[], with hop count and
   total distance.
*/
void print_path(int parent[], int end)
{
    int path[MAX_NODES];
    int len = 0;

    for (int v = end; v != -1; v = parent[v])
    {
        path[len++] = v;
    }

    int metres = 0;

    printf("  ");

    for (int i = len - 1; i >= 0; i--)
    {
        printf("%s", nodeNames[path[i]]);

        if (i > 0)
        {
            printf(" -> ");
            metres += edge_distance(path[i], path[i - 1]);
        }
    }

    printf("\n  Hops: %d, Distance: %d m\n", len - 1, metres);
}


/*
   BFS: explores level by level, so the first exit dequeued is the
   one reachable through the FEWEST points (ignores metres).
*/
void bfs_nearest_exit(int startNode)
{
    int visited[MAX_NODES] = { 0 };
    int parent[MAX_NODES];
    int queue[MAX_NODES];
    int front = 0, rear = 0;

    for (int i = 0; i < graphNodeCount; i++)
    {
        parent[i] = -1;
    }

    visited[startNode] = 1;
    queue[rear++] = startNode;

    while (front < rear)
    {
        int u = queue[front++];

        if (isExit[u])
        {
            printf("BFS nearest exit (fewest hops): %s\n", nodeNames[u]);
            print_path(parent, u);
            return;
        }

        for (AdjNode* a = floorGraph[u]; a != NULL; a = a->next)
        {
            int v = a->nodeIndex;

            if (!visited[v])
            {
                visited[v] = 1;
                parent[v] = u;
                queue[rear++] = v;
            }
        }
    }

    printf("No exit reachable from %s!\n", nodeNames[startNode]);
}


/*
   Dijkstra (array version, O(V^2)): shortest walk in metres to
   every node, then pick the closest exit.
*/
void dijkstra_nearest_exit(int startNode)
{
    int dist[MAX_NODES];
    int parent[MAX_NODES];
    int done[MAX_NODES] = { 0 };

    for (int i = 0; i < graphNodeCount; i++)
    {
        dist[i] = INF;
        parent[i] = -1;
    }

    dist[startNode] = 0;

    for (int k = 0; k < graphNodeCount; k++)
    {
        /* Pick the unvisited node with the smallest distance */
        int u = -1;

        for (int i = 0; i < graphNodeCount; i++)
        {
            if (!done[i] && (u == -1 || dist[i] < dist[u]))
            {
                u = i;
            }
        }

        if (u == -1 || dist[u] == INF)
        {
            break;
        }

        done[u] = 1;

        /* Relax each neighbour */
        for (AdjNode* a = floorGraph[u]; a != NULL; a = a->next)
        {
            int v = a->nodeIndex;

            if (!done[v] && dist[u] + a->distance < dist[v])
            {
                dist[v] = dist[u] + a->distance;
                parent[v] = u;
            }
        }
    }

    int best = -1;

    for (int i = 0; i < graphNodeCount; i++)
    {
        if (isExit[i] && dist[i] != INF && (best == -1 || dist[i] < dist[best]))
        {
            best = i;
        }
    }

    if (best == -1)
    {
        printf("No exit reachable from %s!\n", nodeNames[startNode]);
        return;
    }

    printf("Dijkstra nearest exit (shortest walk): %s\n", nodeNames[best]);
    print_path(parent, best);
}


/* Recursive DFS marking every node reachable from startNode */
void dfs_reachable(int startNode, int visited[])
{
    visited[startNode] = 1;

    for (AdjNode* a = floorGraph[startNode]; a != NULL; a = a->next)
    {
        if (!visited[a->nodeIndex])
        {
            dfs_reachable(a->nodeIndex, visited);
        }
    }
}


/*
   Block one or more corridors (e.g. fire or smoke), DFS outward
   from every exit, and report rooms that can no longer reach any
   exit. The graph is undirected, so "exit can reach room" means
   "room can reach exit". Blocked corridors are restored after.
*/
void check_blocked_corridors(void)
{
    printf("Points in the building:\n");

    for (int i = 0; i < graphNodeCount; i++)
    {
        if (nodeRoomNo[i] == 0)
        {
            printf("  %2d: %s\n", i, nodeNames[i]);
        }
    }

    int blockedA[MAX_NODES], blockedB[MAX_NODES], blockedDist[MAX_NODES];
    int blockedCount = 0;

    while (blockedCount < MAX_NODES)
    {
        int a = read_int("Blocked corridor - first point (-1 to finish): ");

        if (a == -1)
        {
            break;
        }

        int b = read_int("Blocked corridor - second point: ");

        if (a < 0 || a >= graphNodeCount || b < 0 || b >= graphNodeCount)
        {
            printf("Invalid point.\n");
            continue;
        }

        int distance = remove_edge(a, b);

        if (distance == -1)
        {
            printf("No direct corridor between %s and %s.\n", nodeNames[a], nodeNames[b]);
            continue;
        }

        remove_edge(b, a);

        blockedA[blockedCount] = a;
        blockedB[blockedCount] = b;
        blockedDist[blockedCount] = distance;
        blockedCount++;

        printf("Blocked: %s <-> %s\n", nodeNames[a], nodeNames[b]);
    }

    if (blockedCount == 0)
    {
        return;
    }

    int visited[MAX_NODES] = { 0 };

    for (int i = 0; i < graphNodeCount; i++)
    {
        if (isExit[i] && !visited[i])
        {
            dfs_reachable(i, visited);
        }
    }

    int cutOff = 0;

    for (int i = 0; i < graphNodeCount; i++)
    {
        if (nodeRoomNo[i] != 0 && !visited[i])
        {
            printf("  %s is CUT OFF from all exits\n", nodeNames[i]);
            cutOff++;
        }
    }

    if (cutOff == 0)
    {
        printf("  Every room can still reach an exit.\n");
    }

    /* Restore the blocked corridors */
    for (int i = 0; i < blockedCount; i++)
    {
        add_corridor(blockedA[i], blockedB[i], blockedDist[i]);
    }
}


/* ---- Recursion: fire drill batches with Euclid's GCD ----
   For a fire drill, every floor is split into batches of the SAME
   size, as large as possible, with nobody left over. That size is
   the GCD of the number of students on each floor.

   Euclid: gcd(a, b) = gcd(b, a mod b), and gcd(a, 0) = a.
*/
int gcd(int a, int b, int depth)
{
    printf("  %*sgcd(%d, %d)\n", depth * 2, "", a, b);

    if (b == 0)
    {
        return a;                         /* base case */
    }

    return gcd(b, a % b, depth + 1);      /* recursive case */
}


void fire_drill_batches(void)
{
    int onFloor[NUM_FLOORS + 1] = { 0 };

    for (int i = 0; i < roomCount; i++)
    {
        onFloor[rooms[i].floor] += rooms[i].occupied;
    }

    int g = 0;

    for (int f = 1; f <= NUM_FLOORS; f++)
    {
        printf("Floor %d: %d student(s)\n", f, onFloor[f]);
    }

    printf("Euclid's algorithm:\n");

    for (int f = 1; f <= NUM_FLOORS; f++)
    {
        if (onFloor[f] > 0)
        {
            g = (g == 0) ? onFloor[f] : gcd(g, onFloor[f], 0);
        }
    }

    if (g == 0)
    {
        printf("No students have rooms yet.\n");
        return;
    }

    printf("Batch size: %d students per batch.\n", g);

    for (int f = 1; f <= NUM_FLOORS; f++)
    {
        if (onFloor[f] > 0)
        {
            printf("  Floor %d: %d batch(es)\n", f, onFloor[f] / g);
        }
    }
}


/* ---- Minimum spanning tree: CCTV / LAN cable layout ----
   Run cable so every room, stair and exit point is connected,
   using the least total cable. Cable can only follow corridors,
   so this is the MST of the building graph.

   Prim works on an ADJACENCY MATRIX (built from the adjacency
   lists above); Kruskal works on a sorted EDGE LIST with
   union-find. Both must give the same total length.
*/

int adjMatrix[MAX_NODES][MAX_NODES];    /* 0 = no corridor */


void build_adjacency_matrix(void)
{
    for (int i = 0; i < graphNodeCount; i++)
    {
        for (int j = 0; j < graphNodeCount; j++)
        {
            adjMatrix[i][j] = 0;
        }

        for (AdjNode* a = floorGraph[i]; a != NULL; a = a->next)
        {
            adjMatrix[i][a->nodeIndex] = a->distance;
        }
    }
}


/*
   Prim: grow one tree from node 0. Each step, add the cheapest
   edge joining a node inside the tree to one outside. O(V^2).
*/
void prim_mst(void)
{
    int inTree[MAX_NODES] = { 0 };
    int cost[MAX_NODES];        /* cheapest edge into the tree */
    int parent[MAX_NODES];
    int total = 0;

    build_adjacency_matrix();

    for (int i = 0; i < graphNodeCount; i++)
    {
        cost[i] = INF;
        parent[i] = -1;
    }

    cost[0] = 0;

    printf("Prim's MST (adjacency matrix), cable runs:\n");

    for (int k = 0; k < graphNodeCount; k++)
    {
        int u = -1;

        for (int i = 0; i < graphNodeCount; i++)
        {
            if (!inTree[i] && (u == -1 || cost[i] < cost[u]))
            {
                u = i;
            }
        }

        if (cost[u] == INF)
        {
            printf("Building is not fully connected!\n");
            return;
        }

        inTree[u] = 1;

        if (parent[u] != -1)
        {
            printf("  %-18s - %-18s %3d m\n", nodeNames[parent[u]],
                   nodeNames[u], cost[u]);
            total += cost[u];
        }

        for (int v = 0; v < graphNodeCount; v++)
        {
            if (adjMatrix[u][v] != 0 && !inTree[v] && adjMatrix[u][v] < cost[v])
            {
                cost[v] = adjMatrix[u][v];
                parent[v] = u;
            }
        }
    }

    printf("Total cable: %d m\n", total);
}


typedef struct {
    int u, v, distance;
} Edge;

int ufParent[MAX_NODES];


/* Union-find: root of x's group, with path compression */
int uf_find(int x)
{
    if (ufParent[x] != x)
    {
        ufParent[x] = uf_find(ufParent[x]);
    }

    return ufParent[x];
}


/*
   Kruskal: sort all corridors by length, take each one unless it
   joins two points that are already connected (would make a
   cycle). O(E log E) for the sort.
*/
void kruskal_mst(void)
{
    Edge edges[MAX_NODES * 4];
    int edgeCount = 0;

    /* Each undirected corridor once (u < v) */
    for (int u = 0; u < graphNodeCount; u++)
    {
        for (AdjNode* a = floorGraph[u]; a != NULL; a = a->next)
        {
            if (u < a->nodeIndex && edgeCount < MAX_NODES * 4)
            {
                edges[edgeCount].u = u;
                edges[edgeCount].v = a->nodeIndex;
                edges[edgeCount].distance = a->distance;
                edgeCount++;
            }
        }
    }

    /* Insertion sort by length - only ~40 corridors */
    for (int i = 1; i < edgeCount; i++)
    {
        Edge key = edges[i];
        int j = i - 1;

        while (j >= 0 && edges[j].distance > key.distance)
        {
            edges[j + 1] = edges[j];
            j--;
        }

        edges[j + 1] = key;
    }

    for (int i = 0; i < graphNodeCount; i++)
    {
        ufParent[i] = i;
    }

    int total = 0, used = 0;

    printf("Kruskal's MST (sorted edge list), cable runs:\n");

    for (int i = 0; i < edgeCount && used < graphNodeCount - 1; i++)
    {
        int ru = uf_find(edges[i].u);
        int rv = uf_find(edges[i].v);

        if (ru == rv)
        {
            continue;           /* already connected: skip, avoids a cycle */
        }

        ufParent[ru] = rv;
        used++;
        total += edges[i].distance;

        printf("  %-18s - %-18s %3d m\n", nodeNames[edges[i].u],
               nodeNames[edges[i].v], edges[i].distance);
    }

    printf("Total cable: %d m (%d of %d corridors used)\n", total, used, edgeCount);
}


/* ================================================================
   MODULE 8: SORTING - INSERTION vs SELECTION vs MERGE
   ================================================================
   Both sort by roll number and run on COPIES, so students[] (and
   every index pointing into it) is never disturbed.
*/

long insertionComparisons = 0, insertionShifts = 0;
long selectionComparisons = 0, selectionSwaps = 0;
long mergeComparisons = 0;


/* O(n^2): each pass selects the smallest remaining item and swaps
   it into place. Always n(n-1)/2 comparisons, but at most n-1
   swaps - the fewest writes of the three. */
void selection_sort_students(Student a[], int n)
{
    for (int i = 0; i < n - 1; i++)
    {
        int min = i;

        for (int j = i + 1; j < n; j++)
        {
            selectionComparisons++;

            if (compare_roll(a[j].rollNo, a[min].rollNo) < 0)
            {
                min = j;
            }
        }

        if (min != i)
        {
            Student t = a[i];
            a[i] = a[min];
            a[min] = t;
            selectionSwaps++;
        }
    }
}


/* O(n^2): grow a sorted prefix, shifting larger items right */
void insertion_sort_students(Student a[], int n)
{
    for (int i = 1; i < n; i++)
    {
        Student key = a[i];
        int j = i - 1;

        while (j >= 0)
        {
            insertionComparisons++;

            if (compare_roll(a[j].rollNo, key.rollNo) <= 0)
            {
                break;
            }

            a[j + 1] = a[j];
            insertionShifts++;
            j--;
        }

        a[j + 1] = key;
    }
}


void merge(Student a[], Student temp[], int left, int mid, int right)
{
    int i = left, j = mid + 1, k = left;

    while (i <= mid && j <= right)
    {
        mergeComparisons++;

        if (compare_roll(a[i].rollNo, a[j].rollNo) <= 0)
        {
            temp[k++] = a[i++];
        }
        else
        {
            temp[k++] = a[j++];
        }
    }

    while (i <= mid)
    {
        temp[k++] = a[i++];
    }

    while (j <= right)
    {
        temp[k++] = a[j++];
    }

    for (k = left; k <= right; k++)
    {
        a[k] = temp[k];
    }
}


/* O(n log n): split in half, sort each half, merge */
void merge_sort_students(Student a[], Student temp[], int left, int right)
{
    if (left >= right)
    {
        return;
    }

    int mid = left + (right - left) / 2;

    merge_sort_students(a, temp, left, mid);
    merge_sort_students(a, temp, mid + 1, right);
    merge(a, temp, left, mid, right);
}


Student sortA[MAX_STUDENTS], sortB[MAX_STUDENTS], sortC[MAX_STUDENTS];
Student sortTemp[MAX_STUDENTS];


/* Sort the same data all three ways and record the counts */
void run_all_sorts(int n)
{
    memcpy(sortB, sortA, n * sizeof(Student));
    memcpy(sortC, sortA, n * sizeof(Student));

    insertionComparisons = insertionShifts = 0;
    selectionComparisons = selectionSwaps = 0;
    mergeComparisons = 0;

    insertion_sort_students(sortA, n);
    selection_sort_students(sortC, n);
    merge_sort_students(sortB, sortTemp, 0, n - 1);
}


void compare_sorts(void)
{
    if (studentCount > 0)
    {
        memcpy(sortA, students, studentCount * sizeof(Student));
        run_all_sorts(studentCount);

        printf("Registered students sorted by roll no:\n");

        for (int i = 0; i < studentCount; i++)
        {
            printf("  %-6s %s\n", sortB[i].rollNo, sortB[i].name);
        }

        printf("Insertion sort: %ld comparisons, %ld shifts\n",
               insertionComparisons, insertionShifts);
        printf("Selection sort: %ld comparisons, %ld swaps\n",
               selectionComparisons, selectionSwaps);
        printf("Merge sort:     %ld comparisons\n\n", mergeComparisons);
    }

    /* Same experiment on random roll numbers at growing n.
       Fixed seed so the numbers in the report are reproducible. */
    int sizes[] = { 10, 50, 200 };

    srand(42);

    printf("Random roll numbers (seed 42):\n");
    printf("         ---- Insertion ----   ---- Selection ----   - Merge -\n");
    printf("    n    compares     shifts   compares      swaps    compares\n");

    for (int s = 0; s < 3; s++)
    {
        int n = sizes[s];

        for (int i = 0; i < n; i++)
        {
            snprintf(sortA[i].rollNo, ROLL_LEN, "N%03d", rand() % 1000);
        }

        run_all_sorts(n);

        printf("  %3d  %10ld %10ld %10ld %10ld  %10ld\n", n,
               insertionComparisons, insertionShifts,
               selectionComparisons, selectionSwaps, mergeComparisons);
    }
}


/* ================================================================
   CLEANUP
   ================================================================ */

void free_avl(AVLNode* root)
{
    if (root == NULL)
    {
        return;
    }

    free_avl(root->left);
    free_avl(root->right);
    free(root);
}


void free_all(void)
{
    for (int i = 0; i < studentCount; i++)
    {
        GateNode* p = gateHead[i];

        while (p != NULL)
        {
            GateNode* next = p->next;
            free(p);
            p = next;
        }

        gateHead[i] = gateTail[i] = NULL;
        outNodeOf[i] = NULL;
    }

    while (outHead != NULL)
    {
        OutNode* next = outHead->next;
        free(outHead);
        outHead = next;
    }

    outTail = NULL;

    for (int r = 0; r < roomCount; r++)
    {
        while (cleanFront[r] != NULL)
        {
            CleanNode* next = cleanFront[r]->next;
            free(cleanFront[r]);
            cleanFront[r] = next;
        }

        cleanRear[r] = NULL;
    }

    for (int f = 0; f <= NUM_FLOORS; f++)
    {
        DutyNode* p = floorDuty[f];

        while (p != NULL)
        {
            DutyNode* next = p->next;
            free(p);
            p = next;
        }

        floorDuty[f] = NULL;
    }

    for (int i = 0; i < HASH_SIZE; i++)
    {
        HashNode* p = hashTable[i];

        while (p != NULL)
        {
            HashNode* next = p->next;
            free(p);
            p = next;
        }

        hashTable[i] = NULL;
    }

    free_avl(feeTreeRoot);
    feeTreeRoot = NULL;
    feeTreeSize = 0;

    for (int i = 0; i < graphNodeCount; i++)
    {
        AdjNode* p = floorGraph[i];

        while (p != NULL)
        {
            AdjNode* next = p->next;
            free(p);
            p = next;
        }

        floorGraph[i] = NULL;
    }
}


/* ================================================================
   MAIN PROGRAM
   ================================================================ */

/*
   Prompt for a roll number and look it up in the hash table.
   Prints "Student not found." and returns -1 if absent.
*/
int prompt_student(void)
{
    char roll[ROLL_LEN];

    read_roll("Enter roll no (e.g. N254): ", roll);

    int index = hash_lookup(roll);

    if (index == -1)
    {
        printf("Student not found.\n");
    }

    return index;
}


int prompt_floor(void)
{
    return read_int_range("Enter floor (1-3): ", 1, NUM_FLOORS);
}


void print_menu(void)
{
    printf("\n===== %s HOSTEL MANAGEMENT SYSTEM (Warden Console) =====\n", hostelType);
    printf("--- Students ---\n");
    printf("1.  Register Student\n");
    printf("2.  Search Student (linear)\n");
    printf("3.  Search Student (binary)\n");
    printf("4.  Search Student (hash table - chaining)\n");
    printf("5.  Who Is This Phone Number? (hash table - open addressing)\n");
    printf("--- Rooms ---\n");
    printf("6.  Allot Room\n");
    printf("7.  Undo Last Allotment (stack)\n");
    printf("8.  Add to Room Waiting List\n");
    printf("9.  Allot Next from Waiting List\n");
    printf("10. View Rooms\n");
    printf("--- Room Cleaning ---\n");
    printf("11. Record Today's Cleaning (asks student)\n");
    printf("12. View Room Cleaning Record\n");
    printf("--- Complaints ---\n");
    printf("13. File Complaint (heap priority queue)\n");
    printf("14. Resolve Next Complaint (most urgent first)\n");
    printf("15. View All Pending Complaints (heap sort)\n");
    printf("--- Staff ---\n");
    printf("16. Assign Staff to Floor\n");
    printf("17. Remove Staff from Floor\n");
    printf("18. View Floor Duty Roster\n");
    printf("--- Hostel Fee (Rs 1,00,000 / year) ---\n");
    printf("19. Pay Hostel Fee\n");
    printf("20. View Fee Payments (AVL tree, in order paid)\n");
    printf("21. View Pending Fees\n");
    printf("22. Cancel Fee Payment (AVL delete)\n");
    printf("--- Laundry (Rs 500 / month, 75 clothes) ---\n");
    printf("23. Laundry for This Month\n");
    printf("24. View Laundry Status\n");
    printf("--- Gate (curfew %d PM) ---\n", curfewHour - 12);
    printf("25. Student Going Out\n");
    printf("26. Student Coming In\n");
    printf("27. Students Currently Outside (warden's office)\n");
    printf("28. View Student Gate History\n");
    printf("--- Fire Safety ---\n");
    printf("29. Nearest Exit from Room (BFS - fewest hops)\n");
    printf("30. Nearest Exit from Room (Dijkstra - shortest metres)\n");
    printf("31. Blocked Corridor Check (DFS)\n");
    printf("32. Fire Drill Batches (recursive GCD)\n");
    printf("--- CCTV / LAN Cabling ---\n");
    printf("33. Minimum Cable Layout (Prim - adjacency matrix)\n");
    printf("34. Minimum Cable Layout (Kruskal - edge list)\n");
    printf("--- Warden Tools ---\n");
    printf("35. Accounts Calculator (stack: brackets, postfix, evaluate)\n");
    printf("36. Compress a Notice (Huffman coding)\n");
    printf("--- Analysis ---\n");
    printf("37. Sort Students (insertion vs selection vs merge)\n");
    printf("0.  Exit\n");
}


int main(void)
{
    printf("Is this a boys or girls hostel?\n");

    if (read_int_range("1 = Boys (curfew 10 PM), 2 = Girls (curfew 9 PM): ", 1, 2) == 2)
    {
        hostelType = "GIRLS";
        curfewHour = 21;
    }
    else
    {
        hostelType = "BOYS";
        curfewHour = 22;
    }

    initialize_rooms();
    initialize_staff();
    build_floor_graph();

    int choice;

    do
    {
        print_menu();
        choice = read_int("Choice: ");

        switch (choice)
        {
            case 1:
            {
                Student s;

                read_roll("Enter roll no (e.g. N254): ", s.rollNo);

                if (hash_lookup(s.rollNo) != -1)
                {
                    printf("Student with roll number %s already exists.\n", s.rollNo);
                    break;
                }

                do
                {
                    read_line("Enter name: ", s.name, sizeof s.name);
                } while (s.name[0] == '\0');

                read_phone("Student phone: ", s.phone);
                read_phone("Parent phone 1: ", s.parentPhone1);
                read_phone("Parent phone 2: ", s.parentPhone2);

                s.roomNo = 0;          /* Initially student has no room */
                s.hostelFeePaid = 0;   /* Initially fee is pending */
                s.feePaidOn = 0;
                s.laundryMonth = 0;

                if (add_student(s))
                {
                    printf("Student registered successfully.\n");
                }

                break;
            }

            case 2:
            case 3:
            case 4:
            {
                char roll[ROLL_LEN];

                read_roll("Enter roll no to search: ", roll);

                if (choice == 2)
                {
                    print_student(linear_search_student(roll));
                }
                else if (choice == 3)
                {
                    print_student(binary_search_student(roll));
                }
                else
                {
                    print_student(hash_lookup(roll));
                }

                break;
            }

            case 5:
            {
                char phone[PHONE_LEN];

                read_phone("Phone number: ", phone);
                find_by_phone(phone);
                break;
            }

            case 6:
            {
                int index = prompt_student();

                if (index != -1)
                {
                    allot_room(index, read_int("Enter room no: "));
                }

                break;
            }

            case 7:
                undo_last_allotment();
                break;

            case 8:
            {
                int index = prompt_student();

                if (index == -1)
                {
                    break;
                }

                if (students[index].roomNo != 0)
                {
                    printf("%s already has Room %d.\n", students[index].name,
                           students[index].roomNo);
                }
                else if (is_in_waiting_list(index))
                {
                    printf("%s is already on the waiting list.\n", students[index].name);
                }
                else
                {
                    enqueue_waiting(index);
                }

                break;
            }

            case 9:
            {
                if (wlCount == 0)
                {
                    printf("Waiting list is empty.\n");
                    break;
                }

                int roomIndex = find_free_room();

                if (roomIndex == -1)
                {
                    printf("No free beds. Waiting list unchanged.\n");
                    break;
                }

                allot_room(dequeue_waiting(), rooms[roomIndex].roomNo);
                break;
            }

            case 10:
                print_rooms();
                break;

            case 11:
            {
                int index = prompt_student();

                if (index != -1)
                {
                    record_cleaning(index);
                }

                break;
            }

            case 12:
                print_cleaning_record(read_int("Enter room no: "));
                break;

            case 13:
            {
                int index = prompt_student();

                if (index == -1)
                {
                    break;
                }

                if (students[index].roomNo == 0)
                {
                    printf("Student has no room allotted.\n");
                    break;
                }

                Complaint c;

                c.studentIndex = index;
                c.roomNo = students[index].roomNo;
                c.resolved = 0;

                read_line("Describe the problem in your room: ", c.description,
                          sizeof c.description);

                c.priority = (Priority)read_int_range(
                    "Priority (1 = HIGH, 2 = MEDIUM, 3 = LOW): ", COMP_HIGH, COMP_LOW);
                c.complaintId = nextComplaintId++;

                complaint_enqueue(c);
                break;
            }

            case 14:
            {
                if (complaintCount == 0)
                {
                    printf("No pending complaints.\n");
                    break;
                }

                Complaint c = complaint_dequeue();
                c.resolved = 1;

                printf("Resolving complaint #%d [%s]\n", c.complaintId,
                       priority_name(c.priority));
                printf("  Room %d (%s): %s\n", c.roomNo,
                       students[c.studentIndex].rollNo, c.description);

                int roomIndex = find_room(c.roomNo);

                if (roomIndex != -1)
                {
                    print_floor_duty(rooms[roomIndex].floor);
                }

                printf("%d complaint(s) still pending.\n", complaintCount);
                break;
            }

            case 15:
                print_all_complaints();
                break;

            case 16:
            {
                print_staff_list();

                int staffId = read_int("Enter staff ID: ");

                assign_staff_to_floor(prompt_floor(), staffId);
                break;
            }

            case 17:
            {
                int floor = prompt_floor();

                print_floor_duty(floor);

                if (floorDuty[floor] != NULL)
                {
                    remove_staff_from_floor(floor, read_int("Enter staff ID: "));
                }

                break;
            }

            case 18:
                print_floor_duty(prompt_floor());
                break;

            case 19:
            case 22:
            case 23:
            {
                int index = prompt_student();

                if (index == -1)
                {
                    break;
                }

                if (choice == 19)
                {
                    pay_hostel_fee(index);
                }
                else if (choice == 22)
                {
                    cancel_fee_payment(index);
                }
                else
                {
                    pay_laundry(index);
                }

                break;
            }

            case 20:
                print_fee_payments();
                break;

            case 21:
                print_pending_fees();
                break;

            case 24:
                print_laundry_status();
                break;

            case 25:
            case 26:
            case 28:
            {
                int index = prompt_student();

                if (index == -1)
                {
                    break;
                }

                if (choice == 25)
                {
                    going_out(index);
                }
                else if (choice == 26)
                {
                    coming_in(index);
                }
                else
                {
                    print_gate_log_recent_first(index);
                }

                break;
            }

            case 27:
                print_students_outside();
                break;

            case 29:
            case 30:
            {
                int node = room_to_node(read_int("Enter room no: "));

                if (node == -1)
                {
                    printf("Room not found.\n");
                }
                else if (choice == 29)
                {
                    bfs_nearest_exit(node);
                }
                else
                {
                    dijkstra_nearest_exit(node);
                }

                break;
            }

            case 31:
                check_blocked_corridors();
                break;

            case 32:
                fire_drill_batches();
                break;

            case 33:
                prim_mst();
                break;

            case 34:
                kruskal_mst();
                break;

            case 35:
                hostel_calculator();
                break;

            case 36:
                compress_notice();
                break;

            case 37:
                compare_sorts();
                break;

            case 0:
                printf("Exiting...\n");
                break;

            default:
                printf("Invalid choice.\n");
        }

    } while (choice != 0);

    free_all();

    return 0;
}
