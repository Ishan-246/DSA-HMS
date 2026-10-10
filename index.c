#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_STUDENTS 200
#define MAX_ROOMS 100
#define MAX_STAFF 50
#define MAX_COMPLAINTS 100
#define HASH_SIZE 101
#define MAX_NAME 50
#define MAX_UNDO 100
#define MAX_NODES 120

/* ================================================================
   CORE ENTITIES
   ================================================================ */

typedef struct {
    int rollNo;
    char name[MAX_NAME];
    int roomNo;          /* 0 = not allotted */
    int feeDueDay;
    int feesPaid;        /* 0 = pending, 1 = paid */
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
    int assignedFloor;
} Staff;

typedef enum {
    COMP_LOW = 3,
    COMP_MEDIUM = 2,
    COMP_HIGH = 1
} Priority;

typedef struct {
    int complaintId;
    int rollNo;
    int roomNo;
    char description[100];
    Priority priority;
    int resolved;
} Complaint;


/* ================================================================
   GLOBAL STORAGE
   ================================================================ */

Student students[MAX_STUDENTS];
int studentCount = 0;

Room rooms[MAX_ROOMS];
int roomCount = 0;

Staff staff[MAX_STAFF];
int staffCount = 0;


/* ================================================================
   FUNCTION PROTOTYPES
   ================================================================ */

/* Module 1 */
int linear_search_student(int rollNo);
int binary_search_student(int rollNo);
int add_student(Student s);

/* Module 2 */
void initialize_rooms(void);
int find_room(int roomNo);
void push_undo(int rollNo, int previousRoomNo);
void allot_room(int rollNo, int roomNo);
void undo_last_allotment(void);


/* ================================================================
   MODULE 1: ARRAYS + SEARCH
   ================================================================ */

/*
   Add a student to the students array.

   Returns:
   1 -> student successfully added
   0 -> student not added
*/
int add_student(Student s)
{
    /* Check for duplicate roll number */
    if (linear_search_student(s.rollNo) != -1)
    {
        printf("Student with roll number %d already exists.\n",
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
    students[studentCount] = s;

    studentCount++;

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
int linear_search_student(int rollNo)
{
    for (int i = 0; i < studentCount; i++)
    {
        if (students[i].rollNo == rollNo)
        {
            return i;
        }
    }

    return -1;
}


/*
   Binary Search

   IMPORTANT:
   students[] must be sorted by rollNo.

   Returns:
   index -> if student is found
   -1    -> if student is not found

   Time Complexity: O(log n)
*/
int binary_search_student(int rollNo)
{
    int left = 0;
    int right = studentCount - 1;

    while (left <= right)
    {
        int mid = left + (right - left) / 2;

        /* Student found */
        if (students[mid].rollNo == rollNo)
        {
            return mid;
        }

        /* Search right half */
        if (students[mid].rollNo < rollNo)
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


/* ================================================================
   MODULE 2: STACK + ROOM ALLOTMENT + UNDO
   ================================================================ */

/*
   Each stack entry stores one room allotment action.
*/
typedef struct
{
    int rollNo;
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

   Currently we are using one room:
   Room 101
   Capacity = 4
   Floor = 1
*/
void initialize_rooms(void)
{
    rooms[0].roomNo = 101;
    rooms[0].capacity = 4;
    rooms[0].occupied = 0;
    rooms[0].floor = 1;

    roomCount = 1;
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
   Push an allotment action onto the undo stack.

   Stack follows LIFO:
   Last In -> First Out
*/
void push_undo(int rollNo, int previousRoomNo)
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
    undoStack[undoTop].rollNo = rollNo;
    undoStack[undoTop].previousRoomNo = previousRoomNo;
}


/*
   Allot a room to a student.
*/
void allot_room(int rollNo, int roomNo)
{
    /* Find student */
    int studentIndex = linear_search_student(rollNo);

    if (studentIndex == -1)
    {
        printf("Student not found.\n");
        return;
    }


    /* Find room */
    int roomIndex = find_room(roomNo);

    if (roomIndex == -1)
    {
        printf("Room not found.\n");
        return;
    }


    /* Check whether student already has a room */
    if (students[studentIndex].roomNo != 0)
    {
        printf("Student already has Room %d.\n",
               students[studentIndex].roomNo);

        return;
    }


    /* Check room capacity */
    if (rooms[roomIndex].occupied >=
        rooms[roomIndex].capacity)
    {
        printf("Room %d is full.\n", roomNo);

        return;
    }


    /*
       Save previous state before making the change.

       Currently previousRoomNo will normally be 0,
       because students without a room have roomNo = 0.
    */
    push_undo(
        rollNo,
        students[studentIndex].roomNo
    );


    /* Allot room */
    students[studentIndex].roomNo = roomNo;

    /* Increase room occupancy */
    rooms[roomIndex].occupied++;

    printf("Room %d allotted successfully.\n", roomNo);
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


    /* Find student */
    int studentIndex =
        linear_search_student(action.rollNo);

    if (studentIndex == -1)
    {
        printf("Student not found.\n");
        return;
    }


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


    printf("Last room allotment undone.\n");
}


/* ================================================================
   MAIN PROGRAM
   ================================================================ */

int main(void)
{
    initialize_rooms();

    int choice;

    do
    {
        printf("\n");
        printf("========================================\n");
        printf("       HOSTEL MANAGEMENT SYSTEM\n");
        printf("========================================\n");

        printf("1. Register Student\n");
        printf("2. Search Student (Linear)\n");
        printf("3. Search Student (Binary)\n");
        printf("4. Allot Room\n");
        printf("5. Undo Last Allotment\n");
        printf("6. Exit\n");

        printf("Enter choice: ");
        scanf("%d", &choice);


        switch (choice)
        {

            /* ------------------------------------------------
               CASE 1: REGISTER STUDENT
               ------------------------------------------------ */
            case 1:
            {
                Student s;

                printf("Enter roll no: ");
                scanf("%d", &s.rollNo);

                printf("Enter name: ");
                scanf("%49s", s.name);

                printf("Enter fee due day: ");
                scanf("%d", &s.feeDueDay);

                /*
                   Initially student has no room.
                */
                s.roomNo = 0;

                /*
                   Initially fees are pending.
                */
                s.feesPaid = 0;


                /*
                   add_student() returns:

                   1 -> success
                   0 -> failure
                */
                if (add_student(s))
                {
                    printf("Student registered successfully.\n");
                }

                break;
            }


            /* ------------------------------------------------
               CASE 2: LINEAR SEARCH
               ------------------------------------------------ */
            case 2:
            {
                int rollNo;

                printf("Enter roll no to search: ");
                scanf("%d", &rollNo);


                int index =
                    linear_search_student(rollNo);


                if (index == -1)
                {
                    printf("Student not found.\n");
                }
                else
                {
                    printf("Student found!\n");

                    printf("Roll no: %d\n",
                           students[index].rollNo);

                    printf("Name: %s\n",
                           students[index].name);

                    printf("Fee due day: %d\n",
                           students[index].feeDueDay);

                    printf("Room: %d\n",
                           students[index].roomNo);
                }

                break;
            }


            /* ------------------------------------------------
               CASE 3: BINARY SEARCH
               ------------------------------------------------ */
            case 3:
            {
                int rollNo;

                printf("Enter roll no to search: ");
                scanf("%d", &rollNo);


                int index =
                    binary_search_student(rollNo);


                if (index == -1)
                {
                    printf("Student not found.\n");
                }
                else
                {
                    printf("Student found!\n");

                    printf("Roll no: %d\n",
                           students[index].rollNo);

                    printf("Name: %s\n",
                           students[index].name);

                    printf("Fee due day: %d\n",
                           students[index].feeDueDay);

                    printf("Room: %d\n",
                           students[index].roomNo);
                }

                break;
            }


            /* ------------------------------------------------
               CASE 4: ALLOT ROOM
               ------------------------------------------------ */
            case 4:
            {
                int rollNo;
                int roomNo;

                printf("Enter roll no: ");
                scanf("%d", &rollNo);

                printf("Enter room no: ");
                scanf("%d", &roomNo);


                allot_room(rollNo, roomNo);

                break;
            }


            /* ------------------------------------------------
               CASE 5: UNDO ALLOTMENT
               ------------------------------------------------ */
            case 5:
            {
                undo_last_allotment();

                break;
            }


            /* ------------------------------------------------

                break;
            }


            /* ------------------------------------------------
               INVALID CHOICE
               ------------------------------------------------ */
            default:
            {
                printf("Invalid choice.\n");
            }
        }

    } while (choice != 6);


    return 0;
}
