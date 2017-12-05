#include <unistd.h>
#include <stdio.h>



static bool switch_to_user( uid_t user_id, gid_t gp_id )
{
    if ( ( user_id == 0 ) && ( gp_id == 0 ) )
    {
        return false;
    }

    gid_t gid = getgid();
    uid_t uid = getuid();
    if ( ( ( gid != 0 ) || ( uid != 0 ) ) && ( ( gid != gp_id ) || ( uid != user_id ) ) )
    {
        return false;
    }

    if ( uid != 0 )
    {
        return true;
    }

    if ( ( setgid( gp_id ) < 0 ) || ( setuid( user_id ) < 0 ) )
    {
        return false;
    }

    return true;
}

bool daemonize()
{
    pid_t pid = fork();
    if ( pid < 0 )
    {
        return false;
    }
    else if ( pid > 0 )
    {
        exit( 0 );
    }

    umask( 0 );

    pid_t sid = setsid();
    if ( sid < 0 )
    {
        return false;
    }

    if ( ( chdir( "/" ) ) < 0 )
    {
        /* Log the failure */
        return false;
    }

    close( STDIN_FILENO );
    close( STDOUT_FILENO );
    close( STDERR_FILENO );

    open( "/dev/null", O_RDONLY );
    open( "/dev/null", O_RDWR );
    open( "/dev/null", O_RDWR );
    return true;
}


int main()
{
    uid_t uid = getuid();
    uid_t euid = geteuid();
    printf( "userid is %d, effective userid is: %d\n", uid, euid );
    return 0;
}
