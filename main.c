#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <psp2/ctrl.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/registrymgr.h>
#include <sys/time.h>
#include <vitasdk.h>
#include "graphics.h"

#define printf psvDebugScreenPrintf
int ret;
int pos = 0;

int get_key() {
	static unsigned buttons[] = {
		SCE_CTRL_SELECT,
		SCE_CTRL_START,
		SCE_CTRL_UP,
		SCE_CTRL_RIGHT,
		SCE_CTRL_DOWN,
		SCE_CTRL_LEFT,
		SCE_CTRL_LTRIGGER,
		SCE_CTRL_RTRIGGER,
		SCE_CTRL_TRIANGLE,
		SCE_CTRL_CIRCLE,
		SCE_CTRL_CROSS,
		SCE_CTRL_SQUARE,
	};

	static unsigned prev = 0;
	SceCtrlData pad;
	while (1) {
		memset(&pad, 0, sizeof(pad));
		sceCtrlPeekBufferPositive(0, &pad, 1);
		unsigned newb = prev ^ (pad.buttons & prev);
		prev = pad.buttons;
		for (int i = 0; i < sizeof(buttons)/sizeof(*buttons); ++i)
			if (newb & buttons[i])
				return buttons[i];

		sceKernelDelayThread(1000); // 1ms
	}
}

int WriteFile(char *file, void *buf, int size) {
	SceUID fd = sceIoOpen(file, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	if (fd < 0)
		return fd;

	int written = sceIoWrite(fd, buf, size);

	sceIoClose(fd);
	return written;
}

int ReadFile(char *file, void *buf, int size) {
	SceUID fd = sceIoOpen(file,SCE_O_RDWR, 0777);
	if (fd < 0)
		return fd;

	int read = sceIoRead(fd, buf, size);

	sceIoClose(fd);
	return read;
}

int getFileSize(const char *file) {
	SceUID fd = sceIoOpen(file, SCE_O_RDONLY, 0);
	if (fd < 0)
		return fd;
	int fileSize = sceIoLseek(fd, 0, SCE_SEEK_END);
	sceIoClose(fd);
	return fileSize;
}

void deleteSavedata()
{
        //remove the savedata created at boot:
        sceIoRemove("savedata0:/sce_sys/param.sfo");
        sceIoRemove("savedata0:/sce_sys/sealedkey");
        sceIoRemove("savedata0:/sce_sys/keystone");
        sceIoRemove("savedata0:/sce_sys/_safemem.dat");
        sceIoRemove("savedata0:/sce_sys/sdslot.dat");
}

void accountMenu()
{
    pos = 0;
    char bytes[8] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
    sceRegMgrGetKeyBin("/CONFIG/NP", "account_id", bytes,sizeof(bytes));
again:
    psvDebugScreenClear(0);
    printf("Account ID:\n");
    if (pos == -1) pos=0;
    if (pos == 8) pos=7;
    printf("0x%02X%02X%02X%02X%02X%02X%02X%02X\n",bytes[0],bytes[1],bytes[2],bytes[3],bytes[4],bytes[5],bytes[6],bytes[7]);
    if (pos == 0) printf("  ^^              \n");
    if (pos == 1) printf("    ^^            \n");
    if (pos == 2) printf("      ^^          \n");
    if (pos == 3) printf("        ^^        \n");
    if (pos == 4) printf("          ^^      \n");
    if (pos == 5) printf("            ^^    \n");
    if (pos == 6) printf("              ^^  \n");
    if (pos == 7) printf("                ^^\n");
    printf("\n");
    printf("X: Write to registry\n");
    printf("O: Cancel\n");
    switch(get_key()) {
        case SCE_CTRL_UP:
                bytes[pos]++;
                goto again;
        case SCE_CTRL_DOWN:
                bytes[pos]--;
                goto again;
        case SCE_CTRL_RTRIGGER:
            bytes[pos]+= 0x10;
            goto again;
        case SCE_CTRL_LTRIGGER:
            bytes[pos]-= 0x10;
            goto again;
        case SCE_CTRL_LEFT:
                pos = pos - 1;
                goto again;
        case SCE_CTRL_RIGHT:
                pos = pos + 1;
                goto again;
        case SCE_CTRL_CROSS:
            ret = sceRegMgrSetKeyBin("/CONFIG/NP", "account_id", bytes, sizeof(bytes));
            sceRegMgrSetKeyInt("/CONFIG/NP", "enable_np", 1);
            psvDebugScreenClear(0);
            pos = 3;
            if (ret == 0) printf("Written to registry successfully!\n");
            if (ret < 0) printf("Failed to write to registry, %x\n",ret);
            
            sceIoRemove("ux0:id.dat"); //unlink memory card
            deleteSavedata(); //enable trophy eligibility
            
            sceKernelDelayThread(1000000);
            main();
        case SCE_CTRL_CIRCLE:
            pos = 3;
            main();
	default:
		goto again;
	}

}

void saveAccount()
{
                sceIoMkdir("ux0:/AdvancedAccountSwitcher",0006);
                sceIoMkdir("ux0:/AdvancedAccountSwitcher/accounts",0006);
                
                char path[256];
                char username[256];
                memset(username, 0, 256);
                sceRegMgrGetKeyStr("/CONFIG/SYSTEM", "username", username,sizeof(username));
                
                if (strlen(username) == 0)
                {
                psvDebugScreenClear(0);
                printf("There is no linked account.");
                sceKernelDelayThread(1000000);
                pos = 2;
                main();
                }
                
                sprintf(path, "ux0:/AdvancedAccountSwitcher/accounts/%s",username);
                sceIoMkdir(path,0006);
                
                char aid[8] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
                sceRegMgrGetKeyBin("/CONFIG/NP", "account_id", aid,sizeof(aid));
                sprintf(path, "ux0:/AdvancedAccountSwitcher/accounts/%s/aid.bin",username);
                WriteFile(path,aid,sizeof(aid));
                
                char loginId[256];
                memset(loginId, 0, 256);
                sceRegMgrGetKeyStr("/CONFIG/NP", "login_id", loginId,sizeof(loginId));
                sprintf(path, "ux0:/AdvancedAccountSwitcher/accounts/%s/username.txt",username);
                WriteFile(path,loginId,strlen(loginId));
                
                char password[1048];
                memset(password, 0, 1048);
                sceRegMgrGetKeyStr("/CONFIG/NP", "password", password,sizeof(password));
                sprintf(path, "ux0:/AdvancedAccountSwitcher/accounts/%s/password.txt",username);
                WriteFile(path,password,strlen(password));
                
                psvDebugScreenClear(0);
                printf("Account: %s Saved!",username);
                sceKernelDelayThread(1000000);
                main();
}

void switchMenu()
{
    pos = 0;
again:
        psvDebugScreenClear(0);
	printf("Select Account:\n");
        char dirName[256];
        char path[256];

        SceUID dfd = sceIoDopen("ux0:/AdvancedAccountSwitcher/accounts");
        
        int i = 0;
        SceIoDirent dir;
        memset(&dir, 0, sizeof(SceIoDirent));
        while(sceIoDread(dfd, &dir) >0){
        if (pos == i)
        {
            printf(">%s\n",dir.d_name);
            sprintf(dirName,"%s",dir.d_name);
        }
        if (pos != i) printf(" %s\n",dir.d_name);
        i ++;
        }
        sceIoDclose(dfd);
        
        if(i == 0)
        {
        psvDebugScreenClear(0);
        printf("There are no saved accounts!");
        sceKernelDelayThread(1000000);
        pos = 0;
        main();
        }
        
        printf ("\n\nX: Switch to this Account\n");
        printf ("O: Cancel\n");
        
	switch(get_key()) {
	case SCE_CTRL_DOWN:
		pos++;
                if (pos > i-1) pos = i-1;
		goto again;
	case SCE_CTRL_UP:
		pos--;
                if (pos < 0) pos = 0;
		goto again;
        case SCE_CTRL_CROSS:
        
        sceRegMgrSetKeyStr("/CONFIG/SYSTEM", "username", dirName,strlen(dirName));
        char aid[8] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
        sprintf(path, "ux0:/AdvancedAccountSwitcher/accounts/%s/aid.bin",dirName);
        
        ReadFile(path,aid,getFileSize(path));
        sceRegMgrSetKeyBin("/CONFIG/NP", "account_id", aid,sizeof(aid));
        
        char loginId[256];
        memset(loginId, 0, 256);
        sprintf(path, "ux0:/AdvancedAccountSwitcher/accounts/%s/username.txt",dirName);
        ReadFile(path,loginId,getFileSize(path));
        sceRegMgrSetKeyStr("/CONFIG/NP", "login_id", loginId,strlen(loginId));
        
        char password[1048];
        memset(password, 0, 1048);
        sprintf(path, "ux0:/AdvancedAccountSwitcher/accounts/%s/password.txt",dirName);
        ReadFile(path,password,getFileSize(path));
        sceRegMgrSetKeyStr("/CONFIG/NP", "password", password,strlen(password));
        
        sceIoRemove("ux0:id.dat"); //unlink memory card
        deleteSavedata(); //enable trophy eligibility
        
        psvDebugScreenClear(0);
        printf("Switched to account: %s",dirName);
        sceKernelDelayThread(1000000);
        pos = 0;
        main();
        
        case SCE_CTRL_CIRCLE:
            pos = 0;
            main();
	default:
		goto again;
	}
}

void removeAccount()
{
    //setup buffer shit because C sucks.
    char key_buf[8];
    uint64_t uni_key = 0x0000000000000000LL;
    memset(key_buf, 0, sizeof(key_buf));
    memcpy(&key_buf[0], &uni_key, sizeof(key_buf));
    
    //set keys.
    sceRegMgrSetKeyStr("/CONFIG/NP", "login_id", (char[]){0x0}, 1);
    sceRegMgrSetKeyStr("/CONFIG/NP", "password", (char[]){0x0}, 1);
    sceRegMgrSetKeyBin("/CONFIG/NP", "account_id", key_buf, sizeof(key_buf));
    sceRegMgrSetKeyInt("/CONFIG/NP", "enable_np", 0);
    
    sceIoRemove("ux0:id.dat"); //unlink memory card
    deleteSavedata(); //enable trophy eligibility
    
    psvDebugScreenClear(0);
    printf("Account data removed!");
    sceKernelDelayThread(1000000);
    main();
}
void main() {
        int pos_max = 4;
	psvDebugScreenInit();
again:
        if (pos == -1) pos=0;
        if (pos == pos_max+1) pos=pos_max;
        psvDebugScreenClear(0);
        printf("AdvancedAccountSwitcher\n");
        char username[256];
        memset(username, 0, 256);
	sceRegMgrGetKeyStr("/CONFIG/SYSTEM", "username", username,sizeof(username));
        
	if (strlen(username) > 0) printf("Current Account: %s\n", username);
        if (strlen(username) == 0) printf("Current Account: None!\n");


        if (pos == 0) printf(">Switch to a saved account.\n"); else printf(" Switch to a saved account.\n");
	if (pos == 1) printf(">Remove the linked account.\n"); else printf(" Remove the linked account.\n");
        if (pos == 2) printf(">Save the linked account.\n"); else printf(" Save the linked account.\n");
        if (pos == 3) printf(">Change the linked AID.\n"); else printf(" Change the linked AID.\n");
        if (pos == 4) printf(">Run PSN Signup Application.\n"); else printf(" Run PSN Signup Application.\n");

	switch(get_key()) {
	case SCE_CTRL_DOWN:
		pos++;
		goto again;
	case SCE_CTRL_UP:
		pos--;
		goto again;
        case SCE_CTRL_CROSS:

            if (pos == 0)
            {
            switchMenu();
            }
            else if(pos == 1)
            {
            removeAccount();
            }
            else if (pos == 2) 
            {
            saveAccount();
            }
            else if (pos == 3)
            {
            accountMenu();
            }
            else if (pos == 4)
            {
            sceAppMgrLaunchAppByUri(0x20000, "psnreg:");
            goto again;
            }
	default:
		goto again;
	}
}
