#include<stdio.h>
#include<stdlib.h>
#include<string.h>

struct LL {
	struct LL *next;
	char *text;
};

void freeLL(struct LL *node) {
	if (node->next != NULL) {
		freeLL(node->next);
	}
	free(node->text);
	free(node);
}

int main() {
	struct LL *list = malloc(sizeof(struct LL));
	list->next = malloc(sizeof(struct LL));
	list->next->next = NULL;
	list->text = malloc(100);
	list->next->text = malloc(100);
	strcpy(list->text, "Test1");
	strcpy(list->next->text, "Test2");

	freeLL(list);

	/*struct LL *tmp;
	while (list != NULL) {
		tmp = list->next;
		free(list->text);
		free(list);
		list = tmp;
	}*/
}
