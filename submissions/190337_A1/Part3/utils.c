#include "wc.h"


extern struct team teams[NUM_TEAMS];
extern int test;
extern int finalTeam1;
extern int finalTeam2;

int processType = HOST;
const char *team_names[] = {
  "India", "Australia", "New Zealand", "Sri Lanka",   // Group A
  "Pakistan", "South Africa", "England", "Bangladesh" // Group B
};


void teamPlay(void)
{   
    printf("Teamplay %d\n",processType);
    char team_name[14];
    strcat(team_name,teams[processType].name);
    char file_path[40],temp[2];
    int testNumber=test;//1  can change it as per needed as per input (here 1 for working with given testcase)
    file_path[0]='\0';
    sprintf(temp,"%d",testNumber);
    strcat(file_path,"test/");
    strcat(file_path,temp);
    strcat(file_path,"/inp/");
    strcat(file_path,team_name); // finally file_path is in form: test/string(testNumber)/inp/team_name 
    int fd= open(file_path,O_RDONLY); //fd to input file
    int i=0; // for count in input file <=1500
    char write_string[2],check[2];//fine='1';
    while(1){
        i++;
        read(fd,write_string,1);
        write_string[1]='\0';
        write(teams[processType].matchpipe[1],write_string,1);//write next input in matchpipe
        // use commpipe wheter or not to terminate
        // write "1" and read "1" means continue, read "0" means terminate 
        read(teams[processType].commpipe[0],check,1);
        if(!(strcmp(check,"0"))) exit(0); // terminate msg recieved
        if( i == 1501) break; // reached end of file
        write(teams[processType].commpipe[1],"1",1);// match will recieve this and send back 1 i.e. working fine
    }
    exit(0); // after exit goes to swampTeams to end process
}

void endTeam(int teamID)
{
    //write "2" (a exit message) to commpipe telling to terminate the process
    printf("End team %d\n", teamID);
    write(teams[teamID].commpipe[1],"0",1);
}

int match(int team1, int team2)
{   
    printf("Match %d vs %d\n", team1, team2);
    int toss1,toss2,batfirst,bowlfirst,winner;
    char toss_1[2],toss_2[2];
    read(teams[team1].matchpipe[0],toss_1,1); // read toss value team 1
    write(teams[team1].commpipe[1],"1",1); // send msg to continue on writing
    read(teams[team2].matchpipe[0],toss_2,1); // read toss value team 2
    write(teams[team2].commpipe[1],"1",1);
    toss1=atoi(toss_1);
    toss2=atoi(toss_2);
    if((toss1+toss2)%2 == 1) {
        batfirst=team1;
        bowlfirst=team2;
    }
    else {
        batfirst=team2;
        bowlfirst=team1;
    }
    //file name
    char file_path[50],temp[2];
    file_path[0]='\0';
    int testNumber=2;//1; // can change it as per needed (here 1 for working with given testcase)
    sprintf(temp,"%d",testNumber);
    strcat(file_path,"test/");
    strcat(file_path,temp);
    strcat(file_path,"/out/");
    strcat(file_path,teams[batfirst].name);
    strcat(file_path,"v");
    strcat(file_path,teams[bowlfirst].name);
    if(team1<=3 && team2>=4) strcat(file_path,"-Final");
    // open file - "test/(string)testNumber/out/XvY or XvY-Final"
    //NOTE: Create testNumber/out directory beforehand.



    int fd=open(file_path,O_CREAT|O_WRONLY,0644);
    // Innings1 X bats:
    char msg1[50];
    sprintf(msg1, "Innings1: %s bats\n",teams[batfirst].name );
    write(fd,msg1,strlen(msg1));
    //fprintf(file_path,msg1);
    int score1=0,indivscr=0,score2=0,wkt1=0,wkt2=0,batscr,ballscr;

    for(int i=1;i<=120;i++){
        char bat_scr[2],ball_scr[2];
        bat_scr[1]='\0';
        ball_scr[0]='\0';
        read(teams[batfirst].matchpipe[0],bat_scr,1);
        write(teams[batfirst].commpipe[1],"1",1);
        read(teams[bowlfirst].matchpipe[0],ball_scr,1);
        write(teams[bowlfirst].commpipe[1],"1",1);
        ballscr=atoi(ball_scr);
        batscr=atoi(bat_scr);

        if(ballscr != batscr) {
            score1+=batscr;;
            indivscr+=batscr;
        }
        else {
            wkt1++;
            char msg[50];
            sprintf(msg, "%d:%d\n",wkt1,indivscr);
            write(fd,msg,strlen(msg));
            indivscr=0;
        }
        if(wkt1==10) break;
    }
    if(wkt1<10){
        wkt1++;
        char msg[50];
        sprintf(msg, "%d:%d*\n",wkt1,indivscr);
        write(fd,msg,strlen(msg));
        indivscr=0;
    }
    char msg2[50];
    sprintf(msg2, "%s TOTAL: %d\n",teams[batfirst].name,score1);
    write(fd,msg2,strlen(msg2));

    //2nd innings
    char msgi[100];
    sprintf(msgi, "Innings2: %s bats\n",teams[bowlfirst].name);
    write(fd,msgi,strlen(msgi));

    indivscr=0;wkt2=0;score2=0;
    for(int i=1;i<=120;i++){
        char bat_scr[2],ball_scr[2];
        bat_scr[1]='\0';
        ball_scr[0]='\0';
        read(teams[bowlfirst].matchpipe[0],bat_scr,1);
        write(teams[bowlfirst].commpipe[1],"1",1);
        read(teams[batfirst].matchpipe[0],ball_scr,1);
        write(teams[batfirst].commpipe[1],"1",1);
        ballscr=atoi(ball_scr);
        batscr=atoi(bat_scr);

        if(ballscr != batscr) {
            score2+=batscr;;
            indivscr+=batscr;
        }
        else {
            wkt2++;
            char msg[50];
            sprintf(msg,"%d:%d\n",wkt2,indivscr);
            write(fd,msg,strlen(msg));
            indivscr=0;
        }
        if(wkt2==10 || score2>score1) break;

    }
    if(wkt2<10){
        wkt2++;
        char msg[50];
        sprintf(msg,"%d:%d*\n",wkt2,indivscr);
        write(fd,msg,strlen(msg));
    }
    char msg3[50];
    sprintf(msg3 ,"%s TOTAL: %d\n",teams[bowlfirst].name,score2);
    write(fd,msg3,strlen(msg3));

    if(score2 >score1){
        char msg4[100];
        sprintf(msg4,"%s beats %s by %d wickets\n",teams[bowlfirst].name,teams[batfirst].name,10-wkt2+1);
        write(fd,msg4,strlen(msg4));
        winner=bowlfirst;
    }
    else if(score1 >score2){
        char msg4[100];
        sprintf(msg4,"%s beats %s by %d runs\n",teams[batfirst].name,teams[bowlfirst].name,score1-score2);
        write(fd,msg4,strlen(msg4));
        winner=batfirst;
    }
    else {
        char msg4[100];
        sprintf(msg4,"TIE: %s beats %s\n",teams[team1].name,teams[team2].name);
        write(fd,msg4,strlen(msg4));
        winner=team1; // lower index
    }
    return winner;

}

void spawnTeams(void)
{
    for (int i = 0; i < NUM_TEAMS; i++){
        strcpy(teams[i].name, team_names[i]); // get team names
    }
    for (int i = 0; i < NUM_TEAMS; i++){
        printf("Spawn teams: %d\n",i);
        int mpipe=pipe(teams[i].matchpipe);
        int cpipe=pipe(teams[i].commpipe);
        if(mpipe < 0){
            perror("Error: pipe");
            exit(-1);
        }
        if(cpipe < 0){
            perror("Error: pipe");
            exit(-1);
        }
        pid_t Team = fork();
        if(Team < 0 ) {
            perror("Error: fork"); 
            exit(-1);
        }
        if(!Team) {
            processType = i;
            teamPlay(); // after teamPlay gets exited (after recieving msg to end itself)
                        // it reaches here and is followed by process terminates)
            exit(0);
        }
         
        else {continue;} // go to next team process creation 
    }
    return;
	
}

void conductGroupMatches(void)
{   
    int pipe1[2];
    int pipe2[2];
    if(pipe(pipe1) < 0){
        perror("Error: pipe");
        exit(-1);
    }
    if(pipe(pipe2) < 0){
        perror("Error: pipe");
        exit(-1);
    }
    for (int i = 0; i <2;i++) {
        printf("Group process: %d\n", i);
        pid_t groupProcess;
        groupProcess=fork(); // group process
        if(groupProcess< 0 ) {
            perror("Error: fork"); 
            exit(-1);
        }

        if(!groupProcess){ //child group process

            int win_count[4];
            for(int j=0;j<4;j++) win_count[j]=0;
            for(int j=0;j<3;j++) {
                for(int k=j+1;k<4;k++) {
                    int winner= match(4*i+j,4*i+k);
                    win_count[winner%4]++;
                }
            }
            int leader=0;
            for(int j=1;j<4;j++){
              if (win_count[j]>win_count[leader]){
                  leader=j;
              }
            }
            for(int k=0;k<4;k++) {
              if(k!=leader) endTeam(4*i+k);
            }
            char temp[2];
            sprintf(temp,"%d",4*i+leader);
            if(i==0){
                write(pipe1[1],temp,1);
            }
            else {
                write(pipe2[1],temp,1);
            }
            exit(0); // group process terminates

        }
        else { //parent 
            continue;
        }
    }

    wait(NULL);
    char teamfinal1[2],teamfinal2[2];
    read(pipe1[0],teamfinal1,1);
    read(pipe2[0],teamfinal2,1);
    finalTeam1=atoi(teamfinal1);
    finalTeam2=atoi(teamfinal2);

    return;


}