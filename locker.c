#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include "locker.h"

// 0 as unlock, 1 unlock
struct locker_t locker;
int num_of_l= 0;

int childtoParent[2];
int current_l = 0;
int wpipe_to_child[255];
pid_t pids[255];

void childProcess(){
	//printf("im a child!\n");
	while(1){a
		char *msg;
		char buffer[100];
		read(locker.read_fd, buffer, 100);

		if(strcmp(buffer,"DELETE")==0){
			break;
		}
		if(strcmp(buffer,"ownerStatus")==0){
			if(locker.owned == 0){
				msg = "unowned";
				write(locker.write_fd,msg,strlen(msg)+1);
			}else{
				char ret[100];
				snprintf(ret,100,"%u",locker.user_id);
				write(locker.write_fd,ret,strlen(msg)+1);
			}

		}

		if(strcmp(buffer,"lockStatus")== 0){
			if (locker.locked == 0){
				msg = "unlocked";
			}else{
				msg = "locked";
			}
			write(locker.write_fd,msg,strlen(msg)+1);
		}

		if(strcmp(buffer, "ATTACH") == 0){
			read(locker.read_fd, buffer,100);
			int i = strtol(buffer, NULL, 10);

			locker.owned = 1;
			locker.user_id = i;

		}
		if(strcmp(buffer, "DETACH") == 0){
			locker.user_id = -1;
			locker.owned = 0;

		}
	}

	close(locker.read_fd);
	exit(0);
}
void lock(){
	locker.locked = 1;
}
void unlock(){
	locker.locked = 0;
}

int main() {
	pipe(childtoParent);
	char buffer[100];

	//printf("waiting for input...\n");
	while(fgets(buffer, 100, stdin)){
		//printf("got input!\n");
		char* firstArg = strtok(buffer, " \n");
		char* secondArg = strtok(NULL, " \n");

		if(firstArg == NULL){
			printf("Error!\n");
		}
		else if (strcmp(firstArg,"CREATE") == 0){

			int pipefd[2];
			num_of_l++;
			if (num_of_l == 255){
				printf("Cannot Build Locker\n");

			}

			fflush(stdout);

			if (pipe(pipefd) < 0) {
				perror("unable to create pipe");
				return 1;
			 }

			pid_t pid = fork();

			if (pid < 0){
				printf("FORK FAILED!\n");
			}

			else if (pid == 0){
				// child
				locker.id = num_of_l;
				locker.locked = 1;
				locker.owned = 0;
				locker.write_fd = childtoParent[1];
				locker.read_fd = pipefd[0];

				signal(SIGUSR1,lock);
				signal(SIGUSR2,unlock);
				childProcess();

			}else {
				//parent

				pids[num_of_l-1] = pid;
				wpipe_to_child[num_of_l-1] = pipefd[1];
				printf("New Locker Created: %d\n", num_of_l);
				printf("\n");

			}


		}else if(strcmp(firstArg,"DELETE") == 0){
			if(secondArg == NULL){
				printf("please enter locker id\n");
			}else{
				char * ptr;
				int id = strtol(secondArg, &ptr, 10);
				if(wpipe_to_child[id-1] == 0){
					printf("Locker Does Not Exist\n");
				}else{
					char message[100] = "DELETE";

					write(wpipe_to_child[id-1], message, 100);
					printf("Locker %d Removed\n", id);
					printf("\n");
				}
			}


		}else if(strcmp(firstArg, "QUERY") == 0){
			if(secondArg == NULL){
				printf("please enter locker id\n");
			}else{
				char * ptr;
				int id = strtol(secondArg, &ptr, 10);
				if(wpipe_to_child[id-1] == 0){
					printf("Locker Does Not Exist\n");
				}else{
					printf("Locker ID: %d\n", id);
					char buff[100];
					char *message = "lockStatus";

					write(wpipe_to_child[id-1], message, strlen(message)+1);

					read(childtoParent[0],buff, 100);
					printf("Lock Status: %s\n", buff);

					message = "ownerStatus";
					write(wpipe_to_child[id-1], message, strlen(message)+1);
					read(childtoParent[0],buff, 100);
					printf("Owner: %s\n", buff);
					printf("\n");
				}
			}

		}else if(strcmp(firstArg, "QUERYALL") == 0){
			char buff[100];
			for(int i =0; i < num_of_l; i++){
				printf("Locker ID: %d\n", i+1);
				char *message = "lockStatus";

				write(wpipe_to_child[i], message, strlen(message)+1);

				read(childtoParent[0],buff, 100);
				printf("Lock Status: %s\n", buff);

				message = "ownerStatus";
				write(wpipe_to_child[i], message, strlen(message)+1);
				read(childtoParent[0],buff, 100);
				printf("Owner: %s\n", buff);
				printf("\n");
			}


		}else if(strcmp(firstArg, "LOCK") == 0){
			if(secondArg == NULL){
				printf("please enter locker id\n");
			}else{
				char * ptr;
				int id = strtol(secondArg, &ptr, 10);
				if(pids[id-1] == 0){
					printf("Locker Does Not Exist\n");
				}else{
					kill(pids[id-1], SIGUSR1);
					printf("Locker %d locked\n",id);
					printf("\n");
				}
			}

		}else if(strcmp(firstArg, "UNLOCK") == 0){
			if(secondArg == NULL){
				printf("please enter locker id\n");
			}else{
				char * ptr;
				int id = strtol(secondArg, &ptr, 10);
				if(pids[id-1] == 0){
					printf("Locker Does Not Exist\n");
				}else{
					kill(pids[id-1], SIGUSR2);
					printf("Locker %d unlocked\n",id);
					printf("\n");
				}
			}


		}else if(strcmp(firstArg, "ATTACH") == 0){
			// a counter to go to the next in queue
			current_l++;
			if (current_l > num_of_l){
				printf("No Lockers Available\n");
			}else{
			char * ptr;
			int id = strtol(secondArg, &ptr, 10);
			char message[100] = "ATTACH";

			write(wpipe_to_child[current_l-1], message, 100);

			char ownerNum[100];
			snprintf(ownerNum,100,"%d",id);
			write(wpipe_to_child[current_l-1], ownerNum, 100);


			printf("Locker %d Owned By %d\n",current_l, id);
			printf("\n");
		}



		}else if(strcmp(firstArg, "DETACH") == 0){
			char * ptr;
			int id = strtol(secondArg, &ptr, 10);
			char message[100] = "DETACH";

			write(wpipe_to_child[id-1], message, 100);

			printf("Locker %d Unowned\n",id);
			printf("\n");

		}else if(strcmp(firstArg, "QUIT") == 0){

			char message[100] = "DELETE";
			for(int i =0; i<num_of_l; i++){

				write(wpipe_to_child[i], message, 100);
			}

			return 0;
		}else{
			printf("Wrong command!\n");

		}
		//printf("waiting for input!\n");
	}
	close(childtoParent[0]);
	close(childtoParent[1]);
	return 0;
}
