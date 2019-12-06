#ifndef CHORD_H
#define CHORD_H

    struct node {
        int port;
        char address[32];
    };

    struct fingersTable {
        node successor, predecessor;
        node fingers[NMAX];
    };

#endif // CHORD_H
