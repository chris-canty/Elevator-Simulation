#include <unistd.h>
#include <sys/types.h>


int main(){
    for (int i = 0; i < 4; ++i){
        pid_t pid = fork();
        if (pid == 0){
            char * args[] = {"echo", "Syscall"};
            execvp("echo", args);
        }
    }
}