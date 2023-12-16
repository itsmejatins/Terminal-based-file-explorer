#include <cstring>
#include <iostream>
#include <fcntl.h>
#include <fstream>
#include <string>
#include <stack>
#include <iostream>
#include<dirent.h>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <langinfo.h>
#include <locale.h>
#include <termios.h>
#include <stdio.h>
#include <algorithm>
#include <termios.h> // For raw mode
#include <unistd.h> // For raw mode
#include <ctype.h> // For raw mode
#include <stdlib.h> // For raw mode
#include <iomanip>
//#include <stdint.h
#include <sys/ioctl.h>
using namespace std;

bool runNormalMode = false;
bool runCommandMode = false; // program runs till any of the flag is true

string path{}; // renaming path to currentPath can cause errors due to refactoring in setPreviousDirectory function
string homeDirectory{};
stack<string> fstack; // upon pressing ->, take out path from this stack
stack<string> bstack; // upon pressing <-, take out path from this stack

void setAbsolutePath(string &relativePath)
{
    if(relativePath[0] == '~')
    {
        string temp = "/home"; // IMPLEMENT THIS - change /Users to /home : LINUX SPECIFIC MODIFICATION FROM MAC
        for(int i = 1; i < relativePath.size(); i++)
        {
            temp += relativePath[i];
        }
        relativePath = temp;
    }
    
    char buf[PATH_MAX]; /* PATH_MAX incudes the \0 so +1 is not required */
    realpath(&relativePath[0], buf);
    string s = buf;
    relativePath = s;
}

vector<string> splitString(const string &s, char del)
{
    vector<string> splittedString{};
    string element{};
    for(int i = 0; i < s.size() ; i++)
    {
        if(s[i] == del)
        {
            if(element == "")
                continue;
            else
            {
                splittedString.push_back(element);
                element = "";
            }
        }
        else
        {
            element += s[i];
        }
    }
    if(element != "")
        splittedString.push_back(element);
    return splittedString;
}

struct terminalInformation // defining struct to store original terminal state
{
    struct termios originalTermios;
    int nRows{};
    int nCols{};
    int cr{}; // the current position of cursor
};

struct terminalInformation TER; // this object will have current terminal properties

class FileMetadata
{
public:
    string filename{};
    string permissions{};
    string ownerName{}, groupName{};
    unsigned int uid{}, gid{};
    long long fileSize{}; //file size in bytes
    string modified_datestring{};
    
};

class FileMetadataComparator
{
public:
    bool operator()(const FileMetadata &lhs, const FileMetadata &rhs)
    {
        string lhsString = lhs.filename;
        string rhsString = rhs.filename;
        transform(lhsString.begin(), lhsString.end(), lhsString.begin(), ::toupper);
        transform(rhsString.begin(), rhsString.end(), rhsString.begin(), ::toupper);
        return lhsString < rhsString;
    }
};

vector<FileMetadata> currentDirectoryFiles; // global list of files in current directory.

class FileExplorer // FileExplorer is a utility class. One object per vector
{
    
public:
    int p1{0}, p2{}, p3{};
    // p1 -> pointer to the first screen element
    // p2 - pointer to the element currently pointed by cursor
    // p3 -> pointer to the last screen element
    // p1 <= p2 <= p3
    
    void getFileData(string &dirPath) // stores metadata of all the files in the current directory in a vector and returns the sorted version of this vector
    {
        setAbsolutePath(dirPath);
        currentDirectoryFiles.clear();
        bool stat_interrupted = false; // If stat() fails, then probably it is interrupted by OS
        DIR *dir = opendir(&dirPath[0]);
        
        
        if(dir == nullptr)
        {
            cout << "Invalid directory, exiting..." << endl;
            exit(1); //
        }
        struct dirent *entity = readdir(dir);
        while(entity != nullptr) // Store entity information
        {
            struct stat stat_ds; // this stat_ds contains all the information about the file, like inode
            string path = dirPath + "/" + entity->d_name;
            if(dirPath == "/" || dirPath[dirPath.size() - 1] == '/')
            {
                path = dirPath + entity->d_name;
            }
            
            if(stat(&path[0], &stat_ds) == -1)
            {
                //                cout << "Stat not granted permission by OS, exiting" << endl;
                //                exit(1);
                stat_interrupted = true;
            }
            
            FileMetadata metadata;
            
            metadata.filename = entity->d_name;
            //Storing user id
            struct passwd  *pwd;
            pwd = getpwuid(stat_ds.st_uid);
            metadata.ownerName = (pwd == nullptr) ? "NOT_FOUND" : pwd->pw_name;
            metadata.uid = stat_ds.st_uid;
            
            //Storing group id
            struct group   *grp;
            grp = getgrgid(stat_ds.st_gid);
            metadata.groupName = (grp == nullptr) ? "NOT_FOUND" : grp->gr_name;
            
            //Storing filesize
            metadata.fileSize = stat_ds.st_size;
            
            //Storing last modification time
            struct tm *tm;
            tm = localtime(&stat_ds.st_mtime);
            char datestring[256];
            // Get localized date string.
            strftime(datestring, sizeof(datestring), nl_langinfo(D_T_FMT), tm);
            metadata.modified_datestring = datestring;
            
            //Storing permissions
            string temp_perm{};
            temp_perm+=(S_ISDIR(stat_ds.st_mode) ? "d" : "-");
            temp_perm+=((stat_ds.st_mode & S_IRUSR) ? "r" : "-");
            temp_perm+=((stat_ds.st_mode & S_IWUSR) ? "w" : "-");
            temp_perm+=((stat_ds.st_mode & S_IXUSR) ? "x" : "-");
            temp_perm+=((stat_ds.st_mode & S_IRGRP) ? "r" : "-");
            temp_perm+=((stat_ds.st_mode & S_IWGRP) ? "w" : "-");
            temp_perm+=((stat_ds.st_mode & S_IXGRP) ? "x" : "-");
            temp_perm+=((stat_ds.st_mode & S_IROTH) ? "r" : "-");
            temp_perm+=((stat_ds.st_mode & S_IWOTH) ? "w" : "-");
            temp_perm+=((stat_ds.st_mode & S_IXOTH) ? "x" : "-");
            metadata.permissions = temp_perm;
            
            
            currentDirectoryFiles.push_back(metadata);
            
            entity = readdir(dir);
            
        }
        FileMetadataComparator cmp;
        sort(currentDirectoryFiles.begin(), currentDirectoryFiles.end(), cmp);
        
    }
    
    void listFilesNormalMode()
    {
        for(int i = p1; i <= p3; i++)
        {
            if(i == p2)
            {
                FileMetadata f = currentDirectoryFiles.at(i);
                string size = to_string(f.fileSize) + "B";
                string name{};
                for(int i = 0 ; i < min(28, static_cast<int>(f.filename.size())); i++)
                {
                    if(i == 25 || i == 26 || i == 27)
                        name += ".";
                    else
                        name += f.filename.at(i);
                }
                cout << "->" << setw(10) << f.permissions << setw(20) << size << setw(20) << f.ownerName << setw(20)  << f.groupName << setw(30) << f.modified_datestring << "\t\t" << name << "\r\n";
            }
            else
            {
                FileMetadata f = currentDirectoryFiles.at(i);
                string size = to_string(f.fileSize) + "B";
                string name{};
                for(int i = 0 ; i < min(28, static_cast<int>(f.filename.size())); i++)
                {
                    if(i == 25 || i == 26 || i == 27)
                        name += ".";
                    else
                        name += f.filename.at(i);
                }
                cout << "  " << setw(10) << f.permissions << setw(20)  << size << setw(20)  << f.ownerName << setw(20)  <<  f.groupName << setw(30) << f.modified_datestring << "\t\t" << name << "\r\n";
            }
        }
        // This is to print NORMAL MODE AT THE fourth last line
        // Need to print \r\n : total number of rows on screen - total printed rows - 2 times.
        // Total printed lines = (p3-p1+1), total rows = TER.nRows
        // Print NORMAL MODE when loop runs second last time
        for(int i = 0 ; i < (TER.nRows - (p3-p1+1) - 2); i++)
        {
            int loopLastValue = TER.nRows - (p3-p1+1) - 3;
            if(i != loopLastValue - 1) // the second last value
            {
                cout << "\r\n";
            }
            else // print this only at the last time. Rest time print only \r\n
                cout << "[--------NORMAL MODE--------]\r\n";
        }
    }
    
    void listFilesCommandMode()
    {
        for(int i = p1; i <= p3; i++)
        {
            FileMetadata f = currentDirectoryFiles.at(i);
            string size = to_string(f.fileSize) + "B";
            string name{};
            for(int i = 0 ; i < min(28, static_cast<int>(f.filename.size())); i++)
            {
                if(i == 25 || i == 26 || i == 27)
                    name += ".";
                else
                    name += f.filename.at(i);
            }
            cout << "  " << setw(10) << f.permissions << setw(20)  << size << setw(20)  << f.ownerName << setw(20)  <<  f.groupName << setw(30) << f.modified_datestring << "\t\t" << name << "\r\n";
        }
        // This is to print COMMAND MODE AT THE fourth last line
        // Need to print \r\n : total number of rows on screen - total printed rows - 2 times.
        // Total printed lines = (p3-p1+1), total rows = TER.nRows
        // Print NORMAL MODE when loop runs second last time
        for(int i = 0 ; i < (TER.nRows - (p3-p1+1) - 2); i++)
        {
            int loopLastValue = TER.nRows - (p3-p1+1) - 3;
            if(i != loopLastValue - 1) // the second last value
            {
                cout << "\r\n";
            }
            else // print this only at the last time. Rest time print only \r\n
                cout << "[--------COMMAND MODE--------]\r\n";
        }
    }
};


void die(const char *s) // This is the function to be exeucted upon exiting of the program or any failure
{
    // clear screen and reset the cursor to the top left corner of the terminal
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
    perror(s);
    exit(1);
}

void disableRawMode()
{
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &TER.originalTermios) == -1)
        die("tcsetattr");
}
void toggleRawMode()
{
    if (tcgetattr(STDIN_FILENO, &TER.originalTermios) == -1) die("tcgetattr");
    atexit(disableRawMode);
    
    struct termios modifiedTermios = TER.originalTermios; // copy state of original termios struct into modifiedTermios. Now do changes in modifiedTermios and then set those attributes to the terminal.
    
    modifiedTermios.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    modifiedTermios.c_oflag &= ~(OPOST);
    modifiedTermios.c_cflag |= (CS8);
    modifiedTermios.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    modifiedTermios.c_cc[VMIN] = 0;
    modifiedTermios.c_cc[VTIME] = 1;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &modifiedTermios);
}

void resetScreen()
{
    write(STDOUT_FILENO, "\x1b[2J", 4); // clears screen
    write(STDOUT_FILENO, "\x1b[H", 3); // brings cursor to row = 1, col = 1 (1based index)
}
void setCursorPosition(int row, int col) // position cursor to (row, col) on screen, unexpected behavior if row and col exceed terminal dimensions
{
    string s ="\x1b[" + to_string(row) + ";" + to_string(col) + "H\r\n";
    cout << s;
    
}


int storeWindowSize(struct terminalInformation &TER)
{ // using the struct termios present in our manual struct terminalInformation, ncols, nrows are stored in the object of terminal information passed
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        return -1;
    } else {
        int *nRows = &TER.nRows, *nCols = &TER.nCols;
        *nCols = ws.ws_col;
        *nRows = ws.ws_row;
        return 0;
    }
}

void setPreviousDirectory(string &path) // cuts string in path variable
{
    setAbsolutePath(path);
    if(path.size() == 1)
        return;
    int i = static_cast<int>(path.size() - 1);
    if(path[path.size() - 1] == '/') // path should not end with /
        i--;
    // most recent / के पहले तक की string ही रहे path में।
    while(i >=0)
    {
        if(path[i] != '/')
            i--;
        else
            break;
    }
    string previousPath{};
    for(int j = 0; j < i; j++)
    {
        previousPath += path[j];
    }
    // if path is like /jatinsharma
    if(previousPath == "")
        previousPath = "/";
    path = previousPath;
}

// f contains necessary pointers and methods to work with the vector
// call print only to print a new fresh vector, ie, dont use this method in up down keys
// because print reinitializes f corresponding to the new vector, ie, p1,p2,p3 are all reset
void freshPrint(FileExplorer &f) // prints a newly created file data vector
{
    f.p3 = min(static_cast<int>(currentDirectoryFiles.size()) - 1, TER.nRows - 6); f.p1 = 0 ; f.p2 = 0;
    // TER.nRows - 5 because - (see photo in notes)
    // <file list>< ><C/N MODE>< ><commands>< >
    resetScreen();
    if(runNormalMode == true)
        f.listFilesNormalMode();
    else if(runCommandMode == true)
        f.listFilesCommandMode();
}

char readInput()
{
    int nread;
    char c;
    while ((nread = static_cast<int>(read(STDIN_FILENO, &c, 1))) != 1) {
        if (nread == -1 && errno != EAGAIN) die("read");
    }
    if (c == '\x1b') {
        char seq[3];
        if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';
        if (seq[0] == '[') {
            switch (seq[1]) {
                case 'A': return 'w';
                case 'B': return 's';
                case 'C': return 'd';
                case 'D': return 'a';
            }
        }
        return '\x1b';
    } else {
        return c;
    }
}


// remember to print \r\n everytime to reset the cursor to new line col 1
void toggleNormalMode(FileExplorer &f) // keeps processing input and quits on pressing q
{
    write(STDOUT_FILENO, "\x1b[?25l", 6); //hides cursor
    struct termios modifiedTermios = TER.originalTermios; // copy state of original termios struct into modifiedTermios. Now do changes in modifiedTermios and then set those attributes to the terminal.
    
    modifiedTermios.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    modifiedTermios.c_oflag &= ~(OPOST);
    modifiedTermios.c_cflag |= (CS8);
    modifiedTermios.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    modifiedTermios.c_cc[VMIN] = 0;
    modifiedTermios.c_cc[VTIME] = 1;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &modifiedTermios);
    while(true)
    {
        char c = readInput();
        switch (c) {
            case 'q':
                cout << "exiting\r\n" << endl;
                write(STDOUT_FILENO, "\x1b[?25h", 6); //shows cursor
                runNormalMode = false;
                runCommandMode = false;
                return;
                break;
            case ':':
                runNormalMode = false;
                runCommandMode = true;
                return;
            case '\x1b':
                cout <<"escape key pressed \r\n";
                break;
                
            case 's' :
                if(f.p2 >= f.p1 && f.p2 < f.p3) // p2 can move within the window
                {
                    f.p2++;
                    resetScreen();
                    f.listFilesNormalMode();
                    break;
                }
                else if((f.p2 == f.p3) && (f.p3 != currentDirectoryFiles.size() - 1)) // p2 is at the end of window but window can slide forward (ord downward)
                {
                    f.p3++;
                    f.p2++;
                    f.p1++;
                    resetScreen();
                    f.listFilesNormalMode();
                    break;
                }
                else // p2 is at the end of window and window cannot slide forward(downward)
                {
                    break;
                }
            case 'w' :
                if(f.p2 <= f.p3 && f.p2 > f.p1)
                {
                    f.p2--;
                    resetScreen();
                    f.listFilesNormalMode();
                    break;
                }
                else if((f.p2 == f.p1) && (f.p1 != 0))
                {
                    f.p1--;
                    f.p2--;
                    f.p3 --;
                    resetScreen();
                    f.listFilesNormalMode();
                    break;
                }
                else
                {
                    break;
                }
            case 10:
                // case 13 should work.
            case 13: // 13 is enter in mac. CHange it to 10 or 13 depending upon linux machine
                // when i press enter, i cannot go forward
                while(!fstack.empty())
                    fstack.pop();
                if(currentDirectoryFiles.at(f.p2).filename == ".")
                {
                    break;
                }
                else if(currentDirectoryFiles.at(f.p2).filename == "..")
                {
                    c = 127; // Note that case 127 will execute and it will break. case 127 must be cirectly below case 13 or else all the cases will execute since I am not breaking here
                    // this statement is redundant. not breaking means next case will execute even if not matched
                }
                else if(currentDirectoryFiles.at(f.p2).permissions[0] == 'd') // current pointed entity is a directory
                {
                    bstack.push(path);
                    if(path.size() != 1)
                        path = path + "/" + currentDirectoryFiles.at(f.p2).filename;
                    else // for casese like path = /
                        path = path + currentDirectoryFiles.at(f.p2).filename;
                    f.getFileData(path);
                    freshPrint(f);
                    break;
                }
                else
                {
                    if(fork() == 0)
                    {
                        string filePath = path + "/" + currentDirectoryFiles.at(f.p2).filename;
                        execlp("xdg-open", "xdg-open", &filePath[0], NULL);
                        _exit(127);
                    }
                    break;
                }
            case 127:
                // when i press backspace, i cannot go forward
                while(!fstack.empty())
                    fstack.pop();
                if(path.size() == 1)
                    break;
                else
                {
                    bstack.push(path);
                    setPreviousDirectory(path);
                    f.getFileData(path);
                    freshPrint(f);
                    break;
                }
            case 'a':
                if(bstack.empty())
                    break;
                fstack.push(path);
                path = bstack.top(); bstack.pop();
                f.getFileData(path);
                freshPrint(f);
                break;
                
            case 'd':
                if(fstack.empty())
                    break;
                bstack.push(path);
                path = fstack.top(); fstack.pop();
                f.getFileData(path);
                freshPrint(f);
                break;
                
            case 'h':
                while(!fstack.empty())
                    fstack.pop();
                if(path == homeDirectory)
                    break;
                bstack.push(path);
                path = homeDirectory;
                f.getFileData(path);
                freshPrint(f);
                break;
                
            default:
//                cout << "Value is " << (int)c << "\r\n";
                break;
        }
    }
}

bool searchCommandMode(string searchPath, string entityName)
{

    DIR* dir = opendir(&searchPath[0]);
    if(dir == nullptr)
    {
//        cout << "cannot access::" << searchPath << "\r\n";
        return false; // dir = nullptr in case of invalid directories as well as directories for which permission is denied.
    }
    struct dirent *entity = readdir(dir);
    while(entity != nullptr)
    {
        string strDname = entity->d_name;
        if(strDname == entityName)
            return true;
        if(entity->d_type == DT_DIR && strcmp(entity->d_name, ".") != 0 && strcmp(entity->d_name, "..") != 0)
        {
            string inPath{};
            if(searchPath == "/" || searchPath[searchPath.size() - 1] == '/')
            {
                inPath = searchPath + entity->d_name;
            }
            else
            {
                inPath = searchPath + "/" + entity->d_name;
            }
            if(searchCommandMode(inPath, entityName))
                return true;
        }
      
        entity = readdir(dir);
    }
    closedir(dir); // not closing it creates many problems
    return false;
}

// checks whether the given path is a direcotry or not.
bool isDirectory(string &path)
{
    setAbsolutePath(path);
    struct stat stat_ds; // this stat_ds contains all the information about the file, like inode
    
    if(stat(&path[0], &stat_ds) == -1)
        return false;
    if (S_ISDIR(stat_ds.st_mode))
        return true;
    else
        return false;
}

// returns "" if file is valid, and error message if file is invalid
string isValidFile(string &filePath)
{
    setAbsolutePath(filePath);
    if(filePath[filePath.size()-1] == '/')
        return "Invalid file path!";
    string fileParentPath = filePath; setPreviousDirectory(fileParentPath);
    if(!isDirectory(fileParentPath))
        return "Invalid file directory!";
    string fileName{};
    for(int i = static_cast<int>(filePath.size() - 1) ; i >=0; i--)
    {
        if(filePath[i] == '/')
            break;
        fileName = filePath[i] + fileName;
    }
    
    DIR* dir = opendir(&fileParentPath[0]);
    struct dirent *entity = readdir(dir);
    bool brk = false;
    while(entity != nullptr)
    {
        string strEntityDname = entity->d_name;
        if(strEntityDname == fileName && entity->d_type != DT_DIR)
        {
            brk = true;
            break;
        }
        entity = readdir(dir);
    }
    closedir(dir);
    if(!brk)
        return "File does not exist!";
    return "";
}

// copies file (location = filePath) into directory destinationPath. All processing is done inside the function.
string copySingleFile(string &filePath, string &destinationPath)
{
    setAbsolutePath(filePath); setAbsolutePath(destinationPath);
    if(filePath[filePath.size() - 1] == '/')
        return "Invalid file path!";
    // check if destination path is valid
    DIR *dir = opendir(&destinationPath[0]);
    if(dir == nullptr)
        return "Invalid destination path!";
    
    // extract filename
    string fileName{};
    for(int i = static_cast<int>(filePath.size() - 1) ; i >=0 ; i--)
    {
        if(filePath[i] == '/')
            break;
        fileName = filePath[i] + fileName;
    }
    
    // check if file exists in the parent directory
    string fileParentPath = filePath; setPreviousDirectory(fileParentPath);
    DIR *dirF = opendir(&fileParentPath[0]);
    if(dirF == nullptr)
        return "Invalid file path!";
    struct dirent * entity = readdir(dirF);
    bool brk = false;
    while (entity != nullptr)
    {
        string strEntityDname = entity->d_name;
        if(strEntityDname == fileName && entity->d_type != DT_DIR)
        {
            brk = true;
            break;
        }
        entity = readdir(dirF);
    }
    closedir(dirF);
    if(!brk)
        return "Source file does not exist!";
    
    string destinationFilePath{};
    if(destinationPath == "/" || destinationPath[destinationPath.size() - 1] == '/')
        destinationFilePath = destinationPath + fileName;
    else
        destinationFilePath = destinationPath + "/" + fileName;
    
    string message{};
    char copyBlock[1024];
    int src_int{}, dst_int{};
    int nread{};
    src_int = open(&filePath[0], O_RDONLY);
    dst_int = open(&destinationFilePath[0], O_WRONLY|O_CREAT, S_IRUSR|S_IWUSR);
    
    while((nread = read(src_int,copyBlock,static_cast<int>(sizeof(copyBlock)))) > 0)
        write(dst_int,copyBlock,nread);
    
    struct stat srcStat,dstStat;
    if (stat(&filePath[0],&srcStat) != -1) {
    }
    if (stat(&destinationFilePath[0],&dstStat) != -1) {
    }
    
    int status1=chown(&destinationFilePath[0],srcStat.st_uid, srcStat.st_gid);
    if(status1!=0)
        message = "Unable to set ownership of file";
    
    int status2=chmod(&destinationFilePath[0],srcStat.st_mode);
    if(status2!=0)
        message = "Unable to set file permissions";
    
    return message;
}

string copyDirectory(string &sourcePath, string &destinationPath)
{
    setAbsolutePath(sourcePath); setAbsolutePath(destinationPath);
    if(isDirectory(sourcePath) == false || isDirectory(destinationPath) == false)
        return "Invalid directory path!";
    
    // extract source folder name
    int i = static_cast<int>(sourcePath.size() - 1);
    if(sourcePath[sourcePath.size()-1] == '/') i--;
    string sourceFolderName {};
    while(i >= 0)
    {
        if(sourcePath[i] == '/')
            break;
        sourceFolderName = sourcePath[i] + sourceFolderName;
        i--;
    }
        
        // create this folder in destination directory. Using create_dir command implementation
        string dirName = sourceFolderName;
        string dirSubPath = destinationPath;

        string dirPath{}; // the directory of the folder that will be created
        if(dirSubPath[dirSubPath.size() - 1] == '/')
            dirPath = dirSubPath + dirName;
        else
            dirPath = dirSubPath + "/" + dirName;
      
        // ignore if directory already exists
        mkdir(&dirPath[0], S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        if(!isDirectory(dirPath))
            return "Destination directory creation failed!";
        
        DIR* dir = opendir(&sourcePath[0]);
        struct dirent *entity = readdir(dir);
        while(entity != nullptr)
        {
            string strEntityDname = entity->d_name;
            if(strEntityDname == "." || strEntityDname == ".."){entity = readdir(dir); continue;}
            
            // for all directories in the current folder, call this function recursively with source as path of the folder inside orignial folder and destination as newly created folder inside the original folder
            if(entity->d_type == DT_DIR)
            {
                string newSourcePath{};
                if(sourcePath[sourcePath.size() - 1] == '/')
                    newSourcePath = sourcePath + strEntityDname;
                else
                    newSourcePath = sourcePath + "/" + strEntityDname;
                
                string newDestinationPath{};
                if(destinationPath[destinationPath.size() - 1] == '/')
                    newDestinationPath = destinationPath + sourceFolderName;
                else
                    newDestinationPath = destinationPath + "/" + sourceFolderName;
                
                copyDirectory(newSourcePath, newDestinationPath);
            }
            else
            {
                string insideSourceFilePath{};
                if(sourcePath[sourcePath.size() - 1] == '/')
                    insideSourceFilePath = sourcePath + strEntityDname;
                else
                    insideSourceFilePath = sourcePath + "/" + strEntityDname;
                
                string newDestinationPath{};
                if(destinationPath[destinationPath.size() - 1] == '/')
                    newDestinationPath = destinationPath + sourceFolderName;
                else
                    newDestinationPath = destinationPath + "/" + sourceFolderName;
                
                copySingleFile(insideSourceFilePath, newDestinationPath);
            }
            entity = readdir(dir);
        }
        closedir(dir);
    
    return "";
}

string moveFile(string &filePath, string &destinationPath)
{
    string isValidFilePath = isValidFile(filePath);
    if(isValidFilePath != "")
        return isValidFilePath;
    if(!isDirectory(destinationPath))
        return "Invalid destination path!";
    
    string fileName{};
    for(int i = static_cast<int>(filePath.size() - 1) ; i >= 0 ; i--)
    {
        if(filePath[i] == '/')
            break;
        fileName = filePath[i] + fileName;
    }
    string destinationFilePath{};
    if(destinationPath[destinationPath.size() - 1] == '/')
        destinationFilePath = destinationPath + fileName;
    else
        destinationFilePath = destinationPath + "/" + fileName;
    int status = rename(&filePath[0], &destinationFilePath[0]);
    if(status == -1)
        return "Move failed";
    return "";
}

string moveDirectory(string &sourceDir, string &desDir)
{
    if(!isDirectory(sourceDir))
        return "Invalid source directory!";
    if(!isDirectory(desDir))
        return "Invalid destination directory!";
    
    string sourceDirName{};
    int i = static_cast<int>(sourceDir.size() - 1);
    if(sourceDir[i] == '/') i--;
    while(i >= 0)
    {
        if(sourceDir.at(i) == '/')
            break;
        sourceDirName = sourceDir.at(i) + sourceDirName;
        i--;
    }
    
    string modifiedSourceDirPath = sourceDir;
    if(modifiedSourceDirPath.at(modifiedSourceDirPath.size() - 1) == '/')
        modifiedSourceDirPath.at(modifiedSourceDirPath.size() - 1) = '\0';
    string modifiedDesDirPath{};
    if(desDir.at(desDir.size() - 1) == '/')
        modifiedDesDirPath = desDir + sourceDirName;
    else
        modifiedDesDirPath = desDir + "/" + sourceDirName;
    
    int status = rename(&modifiedSourceDirPath[0], &modifiedDesDirPath[0]);
    if(status == -1)
        return "Move failed!";
    return "";
}

string removeSingleFile(string &filePath)
{
    setAbsolutePath(filePath);
    string message = isValidFile(filePath);
    if(message != "")
        return message;
    int status = remove(&filePath[0]);
    if(status != 0)
        return "Failed to delete file::" + filePath;
    return "";
}

string removeSingleDirectory(string &directoryPath)
{
    if(!isDirectory(directoryPath))
        return "Invalid Directory";
    
    DIR *dir = opendir(&directoryPath[0]);
    struct dirent *entity = readdir(dir);
    while(entity != nullptr)
    {
        string strEntityDname = entity->d_name;
        if(strEntityDname == "." || strEntityDname == "..")
        {
            entity = readdir(dir);
            continue;
        }
        if(entity->d_type == DT_DIR)
        {
            string insideDirPath{};
            if(directoryPath[directoryPath.size() - 1] == '/')
                insideDirPath = directoryPath + strEntityDname;
            else
                insideDirPath = directoryPath + "/" + strEntityDname;
            string message = removeSingleDirectory(insideDirPath);
            if(message != "")
                return message;
        }
        else
        {
            string filePath{};
            if(directoryPath[directoryPath.size() - 1] == '/')
                filePath = directoryPath + strEntityDname;
            else
                filePath = directoryPath + "/" + strEntityDname;
            
            string message = removeSingleFile(filePath);
            if(message != "")
                return message;
        }
        entity = readdir(dir);
    }
    closedir(dir);
    int status = rmdir(&directoryPath[0]);
    if(status != 0)
        return "Directory deletion failed";
    return "";
}

void toggleCommandMode(FileExplorer &f)
{
    // need to turn off that timer. Therefore removing vmin, and vtime flags
    struct termios modifiedTermios = TER.originalTermios; // copy state of original termios struct into modifiedTermios. Now do changes in modifiedTermios and then set those attributes to the terminal.
    
    modifiedTermios.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    modifiedTermios.c_oflag &= ~(OPOST);
    modifiedTermios.c_cflag |= (CS8);
    modifiedTermios.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    //    modifiedTermios.c_cc[VMIN] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &modifiedTermios);
    write(STDOUT_FILENO, "\x1b[?25h", 6); //shows cursor
//    resetScreen(); - redundant
    freshPrint(f);
    while(true)
    {
        string input{"$ "};
        write(STDOUT_FILENO, &input[0], input.size());
        while(true)
        {
            char c;
            read(STDIN_FILENO, &c, 1);
            resetScreen();
            freshPrint(f);

            if(c == 13 || c == 10) // LINUX SPECIFIC MODIFICATION FROM MAC
                break;
            
            else if(c == 27)
            {
                runCommandMode = false;
                runNormalMode = true;
                freshPrint(f);
                return;
            }
            else if (c == 127)
            {
                if(input.size() == 2) // dont want to erase "$ "
                {
                    write(STDOUT_FILENO, &input[0], input.size());
                    continue;
                }
                string temp{};
                for(int i = 0; i < input.size() - 1; i++)
                {
                    temp += input[i];
                }
                input = temp;
                resetScreen();
                freshPrint(f);
                write(STDOUT_FILENO, &input[0], input.size());
                continue;
            }
            input += c;
            write(STDOUT_FILENO, &input[0], input.size());
            
        }

        vector<string> sString = splitString(input, ' ');
        
        if(sString.size() == 1) // if input = "$ " and user presses enter
        {
            cout << input << "\r\n";
            cout << "Invalid input\r\n";
        }
        
        else if(sString.at(1) == "quit")
        {
            if(sString.size() != 2)
            {
                cout << input << "\r\n";
                cout << "Invalid command!\r\n";
                continue;
            }
            cout << "\r\n";
            exit(0);
        }
        if(sString.at(1) == "goto")
        {
            if(sString.size() != 3)
            {
                cout << input << "\r\n";
                cout << "Invalid command!\r\n";
                continue;
            }
            
            string gotoPath = sString.at(2);
            setAbsolutePath(gotoPath);
            
            DIR* dir =opendir(&gotoPath[0]);
            if(dir == nullptr)
            {
                cout << input << "\r\n";
                cout << "Invalid directory or permission denied\r\n";
                continue;
            }
            
            bstack.push(path);
            path = gotoPath;
            f.getFileData(path);
            freshPrint(f);
            continue;
        }
        else if(sString.at(1) == "search")
        {
            if(sString.size() != 3)
            {
                cout << input << "\r\n";
                cout << "Invalid command!\r\n";
                continue;
            }
            
            cout << input << "\r\n";
            string entityName = sString.at(2);
            bool b = searchCommandMode(path, entityName);
            
            if(b)
                write(STDOUT_FILENO, "true", 4);
            else
               write(STDOUT_FILENO, "false", 5);
            cout << "\r\n";
            continue;
        }
        else if(sString.at(1) == "create_dir")
        {
            
            if(sString.size() != 4)
            {
                cout << input << "\r\n";
                cout << "Invalid command!\r\n";
                continue;
            }
            
            cout << input << "\r\n";
            string dirName = sString.at(2);
            string dirSubPath = sString.at(3);
            setAbsolutePath(dirSubPath);
            string dirPath{};
            if(dirSubPath == "/" || dirSubPath[dirSubPath.size() - 1] == '/')
                dirPath = dirSubPath + dirName;
            else
                dirPath = dirSubPath + "/" + dirName;
            // check if at provided path, the directory aksed to create already exisits
            DIR *dir = opendir(&dirSubPath[0]);
            if(dir == nullptr)
            {
                cout << "Directory invalid or permission denied\r\n";
                continue;
            }
            struct dirent *entity = readdir(dir);
            bool ex = true;
            while(entity != nullptr)
            {
                string strEntityDname = entity->d_name;
                if(strEntityDname == dirName && entity->d_type == DT_DIR)
                {
                    cout << "Directory already exists\r\n";
                    ex = false;
                    break;
                }
                entity = readdir(dir);
            }
            closedir(dir);
            if(!ex)
                continue;
            
            // create a directory with read/write/search permissions for owner and group, and with read/search permissions for others
            int fail = mkdir(&dirPath[0], S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
            if(fail == -1)
                cout << "Directory creation failed\r\n";
            else // list that directory
            {
                bstack.push(path);
                while(!fstack.empty())
                {
                    fstack.pop();
                }
                f.getFileData(dirPath); // update vector
                path = dirPath;
                freshPrint(f);
                continue;
            }
        
            continue;
        }
        else if(sString.at(1) == "create_file")
        {
            if(sString.size() != 4)
            {
                cout << input << "\r\n";
                cout << "Invalid command!\r\n";
                continue;
            }
            string fileName = sString.at(2);
            string destinationPath = sString.at(3);
            setAbsolutePath(destinationPath);
            
            DIR* dir =opendir(&destinationPath[0]);
            if(dir == nullptr)
            {
                cout << input << "\r\n";
                cout << "Invalid directory or permission denied\r\n";
                continue;
            }
            struct dirent *entity = readdir(dir);
            bool ex = true;
            while(entity != nullptr)
            {
                string strEntityDname = entity->d_name; // to convert char* to string, for == comparison
                if(strEntityDname == fileName && entity->d_type != DT_DIR)
                {
                    cout << input << "\r\n";
                    cout << "File already exists\r\n";
                    ex = false;
                    break;
                }
                entity = readdir(dir);
            }
            closedir(dir);
            if(!ex)
                continue;
            string filePath{};
            if(destinationPath == "/" || destinationPath[destinationPath.size() - 1] == '/') // destination is "/" or "/jatin/"
                filePath = destinationPath + fileName;
            else
                filePath = destinationPath + "/" + fileName;
            int status=open(&filePath[0], O_RDONLY | O_CREAT,S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH );
//            int status = creat(&destinationPath[0],0600);
            if (status == -1)
            {
                cout << input << "\r\n";
                cout << "File creation failed\r\n";
                continue;
            }
            else
            {
                if(path == destinationPath) // show updated list
                {
                    f.getFileData(path);
                    freshPrint(f); // fresh print only fresh_prints the current vector. It does not updates the vector.
                    cout << input << "\r\n";
                    cout << "File successfully created, list updated\r\n";
                    continue;
                }
                else // show current list only, just print message
                {
                    cout << input << "\r\n";
                    cout << "File successfully created\r\n";
                    continue;
                }
            }
        }
        else if(sString.at(1) == "rename")
        {
            if(sString.size() != 4)
            {
                cout << input << "\r\n";
                cout << "Invalid command!\r\n";
                continue;
            }
            
            string sourceFilePath = sString.at(2); setAbsolutePath(sourceFilePath);
            string destinationFilePath = sString.at(3); setAbsolutePath(destinationFilePath);
            // file path must not end with /, becuase then it indicates a directory
            if(sourceFilePath[sourceFilePath.size() - 1] =='/' || destinationFilePath[destinationFilePath.size() - 1] =='/')
            {
                cout << input << "\r\n";
                cout << "Invalid file path!\r\n";
                continue;
            }
            string sourceFileParent = sourceFilePath; setPreviousDirectory(sourceFileParent);
//            check if file exists
            DIR *dir = opendir(&sourceFileParent[0]);
            if(dir == nullptr)
            {
                cout << input << "\r\n";
                cout << "Invalid source directory!\r\n";
                continue;
            }
            string fileName{};
            int i = static_cast<int>(sourceFilePath.size()) - 1;
            while(i >=0)
            {
                if(sourceFilePath[i] != '/')
                {
                    fileName = sourceFilePath[i] + fileName;
                }
                else
                    break;
                i--;
            }
            struct dirent *entity = readdir(dir);
            bool brk = false;
            while(entity != nullptr)
            {
                string strEntityDname = entity->d_name;
                if(strEntityDname == fileName && entity->d_type != DT_DIR)
                {
                    brk = true;
                    break;
                }
                entity = readdir(dir);
            }
            closedir(dir);
            if(!brk)
            {
                cout << input << "\r\n";
                cout << "File does not exist!\r\n";
                continue;
            }
            string destinationFileParent = destinationFilePath; setPreviousDirectory(destinationFileParent);
            if(sourceFileParent != destinationFileParent)
            {
                cout << input << "\r\n";
                cout << "Different path passed in rename!\r\n";
                continue;
            }
            int status = rename(&sourceFilePath[0], &destinationFilePath[0]);
            if(status != 0)
            {
                cout << input << "\r\n";
                cout << "Rename failed!\r\n";
                continue;
            }
            else
            {
                if(path == sourceFileParent)
                {
                    f.getFileData(path);
                    freshPrint(f);
                    cout << input << "\r\n";
                    cout << "Rename success, list updated\r\n";
                    continue;
                }
                else
                {
                    cout << input << "\r\n";
                    cout << "Rename success\r\n";
                    continue;
                }
            }
        }
        else if(sString.at(1) == "copy")
        {
            if(sString.size() < 4)
            {
                cout << input << "\r\n";
                cout << "Invalid command!\r\n";
                continue;
            }
            string destinationPath = sString.at(sString.size() - 1); setAbsolutePath(destinationPath);
            for(int i = 2 ; i < sString.size() - 1; i++)
            {
                string sourcePath = sString.at(i);
                if(isDirectory(sourcePath))
                {
                    string message = copyDirectory(sourcePath, destinationPath);
                    if(message != "")
                    {
                        cout << input << "\r\n";
                        cout << message << "\r\n";
                        break;
                    }
                }
                else
                {
                    string message = copySingleFile(sourcePath, destinationPath);
                    if(message != "")
                    {
                        cout << input << "\r\n";
                        cout << message << "\r\n";
                        break;
                    }
                }
            }
        }
        else if (sString.at(1) == "move")
        {
            if(sString.size() < 4)
            {
                cout << input << "\r\n";
                cout << "Invalid command\r\n";
                continue;
            }
            string destinationPath = sString.at(sString.size() - 1);
            if(!isDirectory(destinationPath))
            {
                cout << input << "\r\n";
                cout << "Invalid destination path!\r\n";
                continue;
            }
            for(int i = 2 ; i < sString.size() - 1; i++)
            {
                string entityPath = sString.at(i);
                if(isDirectory(entityPath))
                {
                    string message = moveDirectory(entityPath, destinationPath);
                    if(message != "")
                    {
                        cout << input << "\r\n";
                        cout << message << "\r\n";
                        break;
                    }
                }
                else if(isValidFile(entityPath) == "")
                {
                    string message = moveFile(entityPath, destinationPath);
                    if(message != "")
                    {
                        cout << input << "\r\n";
                        cout << message << "\r\n";
                        break;
                    }
                }
            }
        }
        else if (sString.at(1) == "delete_file")
        {
            if(sString.size() != 3)
            {
                cout << input << "\r\n";
                cout << "Invalid command\r\n";
                continue;
            }
            string filePath = sString.at(2); setAbsolutePath(filePath);
            string message = removeSingleFile(filePath);
            if(message != "")
            {
                cout << input << "\r\n";
                cout << message << "\r\n";
                continue;
            }
            string fileParentPath = filePath; setPreviousDirectory(fileParentPath);
            if(path == fileParentPath)
            {
                f.getFileData(path);
                freshPrint(f);
                cout << input << "\r\n";
                cout << "Deleted file successfully\r\n";
                continue;
            }
            else
            {
                cout << input << "\r\n";
                cout << "Deleted file successfully\r\n";
                continue;
            }
            
        }
        else if (sString.at(1) == "delete_dir")
        {
            if(sString.size() != 3)
            {
                cout << input << "\r\n";
                cout << "Invalid command\r\n";
                continue;
            }
            string dirPath = sString.at(2); setAbsolutePath(dirPath);
            string message = removeSingleDirectory(dirPath);
            if(message != "")
            {
                cout << input << "\r\n";
                cout << message << "\r\n";
                continue;
            }
            string dirParentDirectory = dirPath; setPreviousDirectory(dirParentDirectory);
            if(path == dirParentDirectory)
            {
                path = dirParentDirectory;
                f.getFileData(path);
                freshPrint(f);
                cout << input << "\r\n";
                cout << "Deleted directory successfully\r\n";
                continue;
            }
            else
            {
                cout << input << "\r\n";
                cout << "Deleted directory successfully\r\n";
                continue;
            }
            
        }
        else
        {
            cout << input << "\r\n";
            cout << "Invalid command!\r\n";
        }
    }
//    runCommandMode = false;
}



// TER is declared.

int main()
{
    path=".";
    setAbsolutePath(path);
    vector<string> splittedPathString = splitString(path, '/');
    homeDirectory = "/" + splittedPathString.at(0) + "/" + splittedPathString.at(1);
    
    
    resetScreen();
    storeWindowSize(TER);
    
    FileExplorer f; // f is a utility object for the global vector currentDirectoryFiles. f contains all the pointer infomration and utility methods like print, reinitiazing the vector for a new path, etc.
    f.getFileData(path);
    runNormalMode = true;
    freshPrint(f);
    write(STDOUT_FILENO, "\x1b[?25l", 6); //hides cursor
    toggleRawMode();
    toggleNormalMode(f);
    while(runCommandMode || runNormalMode)
    {
        if(runCommandMode)
        {
            toggleCommandMode(f);
        }
        if(runNormalMode)
        {
            toggleNormalMode(f);
        }
    }
    
    return 0;
}
