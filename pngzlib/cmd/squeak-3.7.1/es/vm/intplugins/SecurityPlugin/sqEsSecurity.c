#include "sq.h"

/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
static int allowFileAccess = 1;  /* full access to files */

/* directory access */
int ioCanCreatePathOfSize(char* pathString, int pathStringLength)
{
    return 1;
}

int ioCanListPathOfSize(char* pathString, int pathStringLength)
{
    return 1;
}

int ioCanDeletePathOfSize(char* pathString, int pathStringLength)
{
    return 1;
}

/* file access */
int ioCanOpenFileOfSizeWritable(char* pathString, int pathStringLength, int writeFlag)
{
    return 1;
}

int ioCanOpenAsyncFileOfSizeWritable(char* pathString, int pathStringLength, int writeFlag)
{
  return ioCanOpenFileOfSizeWritable(pathString,pathStringLength,writeFlag);
}
int ioCanDeleteFileOfSize(char* pathString, int pathStringLength)
{
    return 1;
}

int ioCanRenameFileOfSize(char* pathString, int pathStringLength)
{
    return 1;
}


int ioCanGetFileTypeOfSize(char* pathString, int pathStringLength)
{
    return 1; /* of no importance here */
}

int ioCanSetFileTypeOfSize(char* pathString, int pathStringLength)
{
    return 1; /* of no importance here */
}

/* disabling/querying */
int ioDisableFileAccess(void)
{
    allowFileAccess = 0;
}

int ioHasFileAccess(void)
{
    return allowFileAccess;
}

/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
/* image security */

static int allowImageWrite = 1;  /* allow writing the image */
int (ioCanRenameImage)(void)
{
    return allowImageWrite; /* only when we're allowed to save the image */
}

int (ioCanWriteImage)(void)
{
    return allowImageWrite;
}

int (ioDisableImageWrite)(void)
{
    allowImageWrite = 0;
}
/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
/* socket security - for now it's all or nothing */
static int allowSocketAccess = 1; /* allow access to sockets */

int ioCanCreateSocketOfType(int netType, int socketType)
{
    return allowSocketAccess;
}

int ioCanConnectToPort(int netAddr, int port)
{
    return allowSocketAccess;
}

int ioCanListenOnPort(void* s, int port)
{
    return allowSocketAccess;
}

int ioDisableSocketAccess()
{
    allowSocketAccess = 0;
}

int ioHasSocketAccess()
{
    return allowSocketAccess;
}

/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
/* SecurityPlugin primitive support */

char *ioGetSecureUserDirectory(void)
{
    return (char *)success(false);
}

char *ioGetUntrustedUserDirectory(void)
{
    return (char *)success(false);
}

/* note: following is called from VM directly, not from plugin */
int ioInitSecurity(void)
{
  return 1;
}

/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
/* private entries for restoring rights */
int _ioSetImageWrite(int enable)
{
    return 1;
}

int _ioSetFileAccess(int enable)
{
    return 1;
}

int _ioSetSocketAccess(int enable)
{
    return 1;
}
