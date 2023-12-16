# Terminal based file explorer

- This is a fully functional File Explorer Application, which runs in two modes -
    - Normal mode (default mode) - used to explore the current directory and navigate the filesystem.
    - Command mode - used to enter shell commands
- The root of your application is same as the system, and the home of the application should be the same as home of the current user.
- The application displays data starting from the top-left corner of the terminal window, line-by- line. The last line of the display screen is to be used as a status bar.

# Normal mode

Normal mode is the default mode of the application. It has the following functionalities.

### Display a list of directories and files in the current folder

- Every file in the directory should be displayed on a new line with the following attributes for each file -
    - File Name
    - File Size
    - Ownership (user and group) and Permissions
    - Last modified
- The file explorer shows entries “.” and “..” for current and parent directory respectively.
- The file explorer handles scrolling using the up and down arrow keys.
- User can navigate up and down in the file list using the corresponding up and down arrow keys. The up and down arrow keys also handles scrolling during
vertical overflow.

### Open directories and files when enter key is pressed

- Directory - Clears the screen and navigate into the directory and shows the directory contents.
- File - Opens the file in vi editor.

### Traversal

1. Go back - Left arrow key takes the user to the previously visited directory. 
2. Go forward - Right arrow key takes the user to the next directory. 
3. Up one level - Backspace key takes the user up one level. 
4. Home – h key takes the user to the home folder. 

# Command mode

The application enters the Command button whenever “:” (colon) key is pressed. In the command mode, the user can enter different commands. All commands appear in the status bar at the bottom.
The commands are as follows - 

| Action | Command |
| --- | --- |
| Copy | $ copy <source_file(s)> <destination_directory> |
| Move | $ move <source_file(s)> <destination_directory> |
| Rename | $ rename <old_filename> <new_filename> |
| Create file | $ create_file <file_name> <destination_path> |
| Create directory | $ create_dir <dir_name> <destination_path> |
| Delete file | $ delete_file <file_path> |
| Delete directory | $ delete_dir <dir_path> |
| Goto | $ goto <location> |
| Search | $ search <file_name> or $ search <directory_name> |

### Switching between modes and quitting the application

On pressing ESC key, the application goes back to Normal Mode and on pressing q key in normal mode, the application closes. Similarly, entering the ‘quit’
command in command mode should also close the application. 

# Miscellaneous

### Behavior of copy command

- it overrides files and directories if already exist in the destination directory. It works as if they never existed, ie, the original files with the same name in the destination directories are overridden.
- appropriate messages are displayed as per the error like 'invalid command', source file does not exist, 'destination file does not exist', failed to create directory' etc.

### Behavior of enter command

Enter key is mapped to 10 or 13 on LINUX depending on machines. Therefore this case is handled by taking or of 10,13, ie, if ASCII value received is 10 or 13 from the keyboard, same action will take place.

### Behavior of move command

If destination already has source files/directory, move will fail.

### Enter key ASCII mapping

Note that in some linux machines (actual machines, not only VM), enter key is mapped to 13 and in some it is mapped to 10. Therefore if character = 10 or 13 is the input from keyboard, for both the inputs, program will behave the same.

### Other

- Normal mode starts from the current directory. Home is the home of the machine.
- Rename command works only for files and not for directories.

# How to run the application

Simply compile and run `main.cpp`.
