

#include <stdio.h>
#include <math.h>

#define INF 100 //infinity
#define NNODE 6

void djikstra(int root);
void print(int root, int *d, int *p);

int linkcost[NNODE][NNODE] = {
    {  0,   2, 5,   1, INF, INF},  // from node-A to other nodes
    {  2,   0, 3,   2, INF, INF},  // from node-B to other nodes
    {  5,   3, 0,   3,   1,   5},  // from node-C to other nodes
    {  1,   2, 3,   0,   1, INF},  // from node-D to other nodes
    {INF, INF, 1,   1,   0,   2},  // from node-E to other nodes
    {INF, INF, 5, INF,   2,   0}   // from node-F to other nodes
};

int dist[NNODE], prev[NNODE];

int main() {
	int root;
	for (root = 0; root < NNODE; root++) {
		djikstra(root);
		print(root, dist, prev);
	}

	return 0;
}

void djikstra(int root) {
	int i, j = 0, k, min = 0, state, node = 0, w, min_s, n[NNODE], boolean;

	for (i = 0; i < NNODE; i++) {
		n[i] = -1;
	}
	
	n[j++] = root;
	
	for (i = 0; i < NNODE; i++) {
		dist[i] = linkcost[root][i];
		prev[i] = root;
	}
	
	while (node < NNODE - 1) {
		state = 0;
		for (i = 0; i < NNODE; i++) {
			boolean = 0;
			for (k = 0; k < NNODE; k++) {
				if (n[k] == i) {
					boolean = 1;
				}
			}
			if (boolean != 1){
				if (state != 0) {
					if (min >= dist[i]){
						if (boolean != 1) {
							min_s = i;
							min = dist[i];
						}
					}
				}
				else {	
					min = dist[i];
					min_s = i;
					state = 1;
				}
			}
		}
		
		for (i = 0; i < NNODE; i++) {
			if (dist[i] > linkcost[min_s][i] + min) {
				dist[i] = linkcost[min_s][i] + min;
				prev[i] = min_s;
			}
		}
		node++;
		n[j++] = min_s;
	}
}

void print(int root, int *d, int *p)
{
	int i;
	char node;
	node = 'A' + root;
	printf("root node %c : \n", node);
	printf("     ");
	for (i = 0; i < NNODE; i++) {
		printf("[%c,%c,%d] ", 'A' + i, prev[i] + 'A' , dist[i]);
	}
	printf("\n\n");
}

		
		
		
	


	
	
	
