#ifndef QUEUE_H
#define QUEUE_H

#include <stdlib.h>
#include <stdio.h>

struct Queue_Element
{
    int lamportTimer;
    int tag;
    struct Queue_Element* next;
};

typedef struct Queue_Element* Node;

struct Queue
{
    int errand_id;
    struct Queue_Element queue;
};

Node createNode()
{
    Node temp;
    temp = (Node)malloc(sizeof(struct Queue_Element));
    temp->next = NULL;
    return temp;
}

Node addNode(Node head, int lamportTimer, int tag)
{
    Node temp, p;
    temp = createNode();
    temp->lamportTimer = lamportTimer;
    temp->tag = tag;
    if(head == NULL)
    {
        head = temp;
    }
    else
    {
        p = head;
        while(p->next != NULL){
            p = p->next;
        }
        p->next = temp;
    }
    return head;
}

Node removeHead(Node head)
{
    if (head == NULL)
        return head;
    Node head2 = head->next;
    free(head);
    return head2;
}

Node removeNode(Node head, int lamportTimer)
{
    if (head == NULL)
        return head;
    else if (head->lamportTimer == lamportTimer)
        return removeHead(head);
    else if (head->next == NULL)
        return head;
    else
    {
        Node p, n;
        p = head;
        while(p->next->lamportTimer != lamportTimer)
        {
            if (p->next->next == NULL)
                return head;
            else
                p = p->next;
        }
        n = p->next;
        p->next = p->next->next;
        free(n);
        return head;
    }
}

void printList(Node head)
{
    if(head == NULL)
    {
        printf("Head is null\n");
        return;
    }
    if(head != NULL)
        printf("Head: %i\n", head->lamportTimer);
    Node p = head;
    while(p->next != NULL)
    {
        printf("Node: %i\n", p->next->lamportTimer);
        p = p->next;
    }
    printf("End of queue !\n");
}

#endif // QUEUE_H