#include<stdio.h>
#include<unistd.h>
#include<string.h>
#include<wait.h>
#include<stdlib.h>


#define BUFSIZE 200
#define ARGVSIZE 40
#define DELIM " \n\t\r"

int main(int argc, char **argv)
{
	int i, n, status, pipePos, isPipe=0, size=BUFSIZE;
	char buffer[size+1];
	char **tokens=malloc(sizeof(char *)*size);
	char *token;
    char *pp="|";
	pid_t wpid;
	while(1)
	{
		//Input Command
		n=0;
		write(STDOUT_FILENO, "> ", 2);
		read(STDIN_FILENO, buffer, size);
		
		if(!strcmp(buffer, "exit\n"))
			exit(0);


		//Tokenize Input
		token=strtok(buffer, DELIM);
		while(token!=NULL)
		{
			tokens[n]=token;
			n++;
			//Realloc tokens[]
			if(n>=size)
			{
				size+=BUFSIZE;
				tokens=realloc(tokens, size *sizeof(char*));
				if(!tokens)
				{
					fprintf(stderr, "sh:Allocation error\n");
					continue;
				}
			}

			token=strtok(NULL, DELIM);
		}
		tokens[n]=NULL;


		//Check if pipe is present
		for(int i=0; i<n; i++) 
		{
            if(!strcmp(tokens[i], pp))
			{
                isPipe=1;
				pipePos=i;
			}
		}

		
		//If cd command
		if(!strcmp(tokens[0], "cd"))
		{
			//$cd
			if(tokens[1]==NULL)
			{
				char path[20]="/home/";
				char *username=getlogin();
				strcat(path, username);
				chdir(path);
			}
			//$cd <path>
			else
			{
				chdir(tokens[1]);
			}
			continue;
		}


		//EXECUTE COMMAND
		//Pipe is not present
		if(!isPipe)
		{	
			pid_t pid;	
			pid=fork();
			//Child Process
			if(pid==0)
			{
				if((execvp(tokens[0], &tokens[0]))<0)
					exit(1);
			}
			//Parent Process
			else
			{
				int status;
				do
				{
					wpid=waitpid(pid, &status, WUNTRACED);
				}while(!WIFEXITED(status) && !WIFSIGNALED(status));

				for(i=0; i<=n; i++)	
					tokens[i]="\0";
				for(i=0; i<size+1; i++)
					buffer[i]='\0';
			}
		}
		//Pipe is present
		else	
		{
			char **pipedTokens=malloc(sizeof(char *)*size);


			//Separate Command
			int j=0;
			tokens[pipePos]=NULL;
			for(int i=pipePos+1; i<n+1; i++)
			{
				pipedTokens[j]=tokens[i];
				j++;
				tokens[i]=NULL;
			}
			pipedTokens[j]=NULL;

		
			//0 is read end, 1 is write end
			int pipefd[2]; 
			pid_t p1, p2;
		
			if(pipe(pipefd)<0) 
			{
				printf("sh:Pipe could not be initialized\n");
				continue;
			}


			p1=fork();
			if(p1<0) 
			{
				printf("sh:Could not fork\n");
				continue;
			}
			else if(p1==0) 
			{
				//Child 1 executing
				//It only needs to write at the write end
				close(pipefd[0]);
				dup2(pipefd[1], STDOUT_FILENO);
				close(pipefd[1]);
		
				if(execvp(tokens[0], &tokens[0])<0) 
				{
					printf("sh:Could not execute command 1\n");
					exit(0);
				}
			} 
			else 
			{
				//Parent executing
				p2=fork();
		
				if(p2<0) 
				{
					printf("sh:Could not fork\n");
					continue;
				}
				//Child 2 executing
				//It only needs to read at the read end
				else if(p2==0) 
				{
					close(pipefd[1]);
					dup2(pipefd[0], STDIN_FILENO);
					close(pipefd[0]);
					if(execvp(pipedTokens[0], &pipedTokens[0])<0) 
					{
						printf("sh:Could not execute command 2\n");
						continue;
					}
				} 
				else 
				{
					//Parent executing, waiting for two children
					int status;
					do
					{
						wpid=waitpid(p1, &status, WUNTRACED);
					}while(!WIFEXITED(status) && !WIFSIGNALED(status));

					
					for(i=0; i<=n; i++)	
						tokens[i]="\0";
					for(i=0; i<size+1; i++)
						buffer[i]='\0';
					isPipe=0;
				}
			}
		}
	}
}