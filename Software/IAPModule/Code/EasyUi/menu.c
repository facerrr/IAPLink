#include "menu.h"
#include "easy_ui.h"
#include "flashx.h"


EasyUIPage_t ZSeries, XSeries;
EasyUIItem_t titileXSerie, titileZSerie;

EasyUIItem_t iapfile1, iapfile2, iapfile3, iapfile4;
EasyUIItem_t iapfile5, iapfile6, iapfile7, iapfile8;

char u8FileNames[8][32];

void MenuInit(void)
{
    EasyUIAddPage(&ZSeries, PAGE_LIST);
    EasyUIAddPage(&XSeries, PAGE_LIST);

    EasyUIAddItem(&ZSeries, &titileZSerie, "ZSeries", ITEM_JUMP_PAGE, XSeries.id);
    EasyUIAddItem(&XSeries, &titileXSerie, "XSeries", ITEM_JUMP_PAGE, ZSeries.id);

    EasyUIItem_t *iapItems[] = {
        &iapfile1, &iapfile2, &iapfile3, &iapfile4,
        &iapfile5, &iapfile6, &iapfile7, &iapfile8
    };
    
    EasyUIPage_t *pages[] = {
        &ZSeries, &ZSeries, &ZSeries, &ZSeries,
        &XSeries, &XSeries, &XSeries, &XSeries
    };
    memset(u8FileNames, 0, 8 * 32);
    for(int i = 0; i < 8; i++) {
        uint32_t fileAppStart = FLASH_BINK2_BASE + i * IAP_FILE_SIZE;
        uint32_t fileInfoStart = fileAppStart + IAP_FILE_APP_SIZE;
        IAP_ReadFileInfo(fileInfoStart, u8FileNames[i], 32);
        EasyUIAddItem(pages[i], iapItems[i], u8FileNames[i], ITEM_MESSAGE, "Downloading...", EasyUIEventIAPFinished);
    }
}