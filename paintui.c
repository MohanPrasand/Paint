#include<stdlib.h>
#include<stdio.h>
#include<graphics.h>
#include<math.h>
#include<windows.h>
#include<commdlg.h>




//bmp reader

#pragma pack(push,1)

void **bmpp;

typedef struct{
    unsigned char sig1;
    unsigned char sig2;
    unsigned int size;
    unsigned short res1;
    unsigned short res2;
    unsigned int start_address;
} header;

typedef struct{
    unsigned int header_size;
    unsigned int width;
    unsigned int height;
    unsigned short int planes;
    unsigned short int bits_per_pixel;
    unsigned int comp_meth;
    unsigned int size;
    unsigned int hres;
    unsigned int vres;
    unsigned int ncolors;
    unsigned int impcolors;
} dibheader;

typedef struct{
    unsigned char b;
    unsigned char g;
    unsigned char r;
    unsigned char res;
} colory;

typedef struct{
    unsigned char len;
    unsigned char color;
} codereader;

int savebmp(char* filename){
    FILE* img= fopen(filename,"wb");
    header head;
    dibheader bid;
    int width=1080,height=720;
    int padding = (4-(width*3)%4)%4;
    int row_size=width*3+padding;
    char* pad[]={0,0,0};
    int i,j;

    head.sig1='B';
    head.sig2='M';
    head.size=sizeof(header)+sizeof(dibheader)+(row_size)*height;
    head.res1=0;
    head.res2=0;
    head.start_address=sizeof(header)+sizeof(dibheader);



    bid.header_size=sizeof(dibheader);
    bid.width=width;
    bid.height=height;
    bid.planes=1;
    bid.bits_per_pixel=24;
    bid.comp_meth=0;
    bid.size=(row_size)*height;
    bid.hres=2835;
    bid.vres=2835;
    bid.ncolors=0;
    bid.impcolors=0;

    

    fwrite(&head,sizeof(header),1,img);
    fwrite(&bid,sizeof(dibheader),1,img);


    colory pixel;
    unsigned char* bmp = (unsigned char*) (*bmpp);
    bmp+=24;

    for(i=height-1;i>=0;i--){
        for(j=0;j<width;j++){
            pixel.b=bmp[((i*1081)+j)*4];
            pixel.g=bmp[((i*1081)+j)*4+1];
            pixel.r=bmp[((i*1081)+j)*4+2];
            fwrite(&pixel,3,1,img);
        }
        fwrite(pad,padding,1,img);
    }
    fclose(img);

    return 0;
}

int decompress(FILE* img, unsigned char* arr,header mainHeader, dibheader dheader){
    unsigned char palette[256][3],col;
    colory pixel;
    codereader code;
    int i,j,x;


    fseek(img,14+dheader.header_size,SEEK_SET);
    for(i=0;i<256;i++){
        fread(&pixel,4,1,img);
        palette[i][0]=pixel.r;
        palette[i][1]=pixel.g;
        palette[i][2]=pixel.b;
    }

    fseek(img,mainHeader.start_address,SEEK_SET);

    fread(&code,2,1,img);
    i=dheader.height-1,j=0;
    while(code.len!=0 || code.color!=1){
        if(code.len==0 && code.color==0){
            i-=1;
            j=0;
        }
        else if(code.len==0){
            x=0;
            while(x<code.color){
                fread(&col,1,1,img);
                arr[(i*dheader.width+j)*3]=palette[col][0];
                arr[(i*dheader.width+j)*3+1]=palette[col][1];
                arr[(i*dheader.width+j)*3+2]=palette[col][2];
                x++;
                j++;
            }
        }
        else{
            x=0;
            while(x<code.len){
                arr[(i*dheader.width+j)*3]=palette[code.color][0];
                arr[(i*dheader.width+j)*3+1]=palette[code.color][1];
                arr[(i*dheader.width+j)*3+2]=palette[code.color][2];
                x++;
                j++;
            }
        }
        fread(&code,2,1,img);
    }
}

int loadbmp(FILE* img, unsigned char** arrp, int *width, int* height){
    size_t readbytes;
    header mainHeader;
    dibheader dheader;
    colory pixel;
    int i,j;

    readbytes=fread(&mainHeader, sizeof(header),1,img);

    readbytes=fread(&dheader, sizeof(dibheader),1,img);


    *width = dheader.width;
    *height = dheader.height;
    *arrp = (unsigned char*) malloc((*width)*(*height)*3);
    unsigned char *arr = *arrp;


    if(dheader.comp_meth==0){
        fseek(img,mainHeader.start_address,SEEK_SET);
        for(i=dheader.height-1;i>=0;i--){
            for(j=0;j<dheader.width;j++){
                readbytes=fread(&pixel,3,1,img); 

                arr[(i*dheader.width+j)*3]=pixel.r;
                arr[(i*dheader.width+j)*3+1]=pixel.g;
                arr[(i*dheader.width+j)*3+2]=pixel.b;
            }
            fseek(img,dheader.width%4,SEEK_CUR);
        }
        return 0;
    }
    else if(dheader.comp_meth==1){
        decompress(img,arr,mainHeader,dheader);
        return 0;
    }
    
    return -1;
}

int loadbmp(char* filename, unsigned char** imgarr, int* width,int* height){
    FILE* img = fopen(filename,"rb");
    if(!img) return -1;
    return loadbmp(img,imgarr, width, height);
}

int displayimg(FILE* img){
    int width,height,i,j,x,y;
    unsigned char *arr;
    loadbmp(img,&arr,&width,&height);
    x=(1080-width)/2;
    y=(720-height)/2;
    if(x<100) x=100;
    if(y<0) y=0;

    unsigned char* nbmp =(unsigned char*) *bmpp;
    nbmp+=24;
    int ioff=0;
    
    for(i=y;i<y+height;i++){
        for(j=x;j<x+width;j++){
            nbmp[(i*1081+j)*4]=arr[ioff+2];
            nbmp[(i*1081+j)*4+1]=arr[ioff+1];
            nbmp[(i*1081+j)*4+2]=arr[ioff];
            ioff+=3;
        }
    }
    putimage(100,0,*bmpp,COPY_PUT);
    return 0;
}


//file selection
char* fileselector(){
    char filename[MAX_PATH]={0};

    OPENFILENAME ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize=sizeof(ofn);
    ofn.hwndOwner=NULL;
    ofn.lpstrFilter = "BMP Files\0*.bmp\0";
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    ofn.lpstrFileTitle = "Select an image to import";

    if(!GetOpenFileName(&ofn))
        return NULL;
    displayimg(fopen(filename,"rb"));
    return filename;
}

int imgselector(int i){
    char* filename = fileselector();
    if(filename) return 0;
    displayimg(fopen(filename,"rb"));
    getimage(100,0,1180,720,*bmpp);
    clearmouseclick(WM_LBUTTONDOWN);
    clearmouseclick(WM_LBUTTONUP);
    return 0;
}

int writeimage(int i){
    OPENFILENAME ofn;
    char filename[MAX_PATH]="";

    ZeroMemory(&ofn,sizeof(ofn));
    ofn.lStructSize=sizeof(ofn);
    ofn.hwndOwner=NULL;
    ofn.lpstrFile=filename;
    ofn.nMaxFile=sizeof(filename);
    ofn.lpstrFilter="BMP Files\0*.bmp\0";
    ofn.nFilterIndex=1;
    ofn.lpstrFileTitle=NULL;
    ofn.nMaxFileTitle=0;
    ofn.lpstrInitialDir = 0;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

    if(GetSaveFileName(&ofn))
        savebmp(filename);
    return 0;
}


//Drawing Tools
int color = BLACK;
int tool = -1;

int solidfill(int x,int y){
    unsigned char* bmp = (unsigned char*)*bmpp;
    bmp+=24;
    x-=100;
    unsigned int tcolor=COLOR(bmp[(y*1081+x)*4],bmp[(y*1081+x)*4+1],bmp[(y*1081+x)*4+2]);
    if(tcolor == color) return 0;
    int sz=2,i=0,len=10000;
    int *q=(int*)malloc(sizeof(int)*len);
    

        


    q[0]=x;
    q[1]=y;

    sz=2,i=0;
    while(abs(i-sz)>=2){
        x=q[i++];
        i%=len;
        y=q[i++];
        i%=len;
        if(x<0 || x>=1080 || y<0 || y>=720 || COLOR(bmp[(y*1081+x)*4],bmp[(y*1081+x)*4+1],bmp[(y*1081+x)*4+2])!=tcolor) continue;
        bmp[(y*1081+x)*4]=(color>>16)&0xff;
        bmp[(y*1081+x)*4+1]=(color>>8)&0xff;
        bmp[(y*1081+x)*4+2]=(color)&0xff;
        

        q[sz++]=x+1;
        sz%=len;
        q[sz++]=y;
        sz%=len;
        q[sz++]=x-1;
        sz%=len;
        q[sz++]=y;
        sz%=len;
        q[sz++]=x;
        sz%=len;
        q[sz++]=y+1;
        sz%=len;
        q[sz++]=x;
        sz%=len;
        q[sz++]=y-1;
        sz%=len;
    }
    putimage(100,0,*bmpp,COPY_PUT);
    return 0;
}

int drawShape(int x, int y, void *bitmap){
    int x1,y1;
    static int i;
    setcolor(color);
    setlinestyle(SOLID_LINE,0,2);
    if(tool==10){
        solidfill(x,y);
        getimage(100,0,1180,720,bitmap);
        return 0;
    }
    while(!ismouseclick(WM_LBUTTONUP)){
        if(ismouseclick(WM_MOUSEMOVE)){
            if(x1<100) continue;
            getmouseclick(WM_MOUSEMOVE,x1,y1);
            putimage(100,0,bitmap,COPY_PUT);
            switch(tool){
                case 0:
                    ellipse((x+x1)/2,(y+y1)/2,0,360,abs((x+x1)/2-x1),abs((y+y1)/2-y1));
                    break;
                case 1:
                    line(x,y,x1,y1);
                    break;
                case 4:
                    rectangle(x,y,x1,y1);
                    break;
                case 3:
                    if(x1-12<=100) continue;
                    setlinestyle(SOLID_LINE,0,10);
                    setcolor(WHITE);
                case 8:
                    line(x,y,x1,y1);
                    x=x1,y=y1;
                    getimage(100,0,1180,720,bitmap);
                    break;
            } 
        }
    }
    getimage(100,0,1180,720,bitmap);
    clearmouseclick(WM_LBUTTONUP);
    return 1;
}

int selecttool(int t){
    tool=t;
    return 0;
}

int selectcolor(int col){
    color = col;
    return 0;
}




//UI Constuctor
typedef struct{
    char type;
    int isFill,x,y,width,height;
    COLORREF  borderColor,fillColor;
} shape;

typedef struct {
    int left,top,width,height;
    int (*action)(int);
    int arg;
    int shapes_count;
    shape shapes[10];
} button;

typedef struct{
    int x1,y1,x2,y2;
    int (*action)(int);
    int arg;
} event;

int copyEvent(event *c,event *cc){
    c->x1=cc->x1;
    c->y1=cc->y1;
    c->x2=cc->x2;
    c->y2=cc->y2;

    c->action=cc->action;
    c->arg=cc->arg;
}

event events[100];
int nevents=0;

int addEvent(button b){
    events[nevents].x1=b.left;
    events[nevents].y1=b.top;
    events[nevents].x2=b.left+b.width;
    events[nevents].y2=b.top+b.height;
    events[nevents].action=b.action;
    events[nevents++].arg=b.arg;
    return 1;
}

int orderEvents(){
    int i,j,k;
    event tmp;
    for(i=0;i<nevents-1;i++){
        k=i;
        for(j=i+1;j<nevents;j++){
            if((events[k].x1>events[j].x1)||(events[k].x1==events[j].x1 && events[k].y1>events[j].y1)) k=j;
        }
        if(k!=j){
            copyEvent(&tmp,events+i);
            copyEvent(events+i,events+k);
            copyEvent(events+k,&tmp);
        }
    }
    return 1;
}

int findevent(int x, int y,void* cmap){
    int l=0,r=nevents-1,mid;
    while(l<=r){
        mid=(l+r)/2;
        if((events[mid].x1<=x && x<=events[mid].x2) && (events[mid].y1<=y && y<=events[mid].y2)){
            events[mid].action(events[mid].arg);
            if(events[mid].y1<160){
                putimage(0,0,cmap,COPY_PUT);
                setlinestyle(SOLID_LINE,0,3);
                setcolor(RGB(31,97,141));
                rectangle(events[mid].x1,events[mid].y1,events[mid].x2,events[mid].y2);
            }
            return 1;
        }
        else if(events[mid].x2<x  || (x>=events[mid].x1 && events[mid].y2<y))
            l=mid+1;
        else
            r=mid-1;
    }
    return 0;
}

void drawshape(shape sh){
    setcolor(sh.borderColor);
    if(sh.type=='r')
        rectangle(sh.x,sh.y,sh.x+sh.width,sh.y+sh.height);
    else if(sh.type=='c')
        ellipse(sh.x,sh.y,0,360,sh.width,sh.height);
    else if(sh.type=='l')
        line(sh.x,sh.y,sh.width,sh.height);
    if(sh.type!='l' && sh.isFill){
        setfillstyle(SOLID_FILL,sh.fillColor);
        floodfill((sh.x*2+sh.width)/2,(sh.y*2+sh.height)/2,sh.borderColor);
    }
}

void drawbutton(button b){
    int i;
    setfillstyle(SOLID_FILL,RGB(0,0,0));
    setcolor(RGB(20,20,20));
    rectangle(b.left,b.top,b.width+b.left,b.height+b.top);
    for(i=0;i<b.shapes_count;i++){
        shape sh=b.shapes[i];
        drawshape(sh);
    }
}

int selectimg(int i){
    return 0;
}

int numbuttons=15;
button buttons[]={
    (button){5,10,40,40,selecttool,4,1,{(shape){'r',0,10,15,30,30,RGB(0,0,0)}}},
    (button){55,10,40,40,selecttool,0,1,{(shape){'c',0,75,30,17,17,RGB(0,0,0)}}},
    (button){5,55,40,40,selecttool,1,1,{(shape){'l',0,40,60,10,90,RGB(0,0,0)}}},
    (button){55,55,40,40,selecttool,8,3,{(shape){'r',1,60,71,30,5,RGB(0,0,0),RGB(200,190,20)},(shape){'r',1,70,56,10,15,RGB(0,0,0),RGB(200,190,20)},(shape){'r',1,60,75,30,15,RGB(0,0,0),RGB(200,200,120)}}},
    (button){5,105,40,40,selecttool,10,1,{(shape){'r',1,10,115,30,30,RGB(0,0,0),RGB(200,50,100)}}},
    (button){55,105,40,40,selecttool,3,2,{(shape){'r',1,70,115,10,5,RGB(0,0,0),RGB(250,250,250)},(shape){'r',1,70,120,10,20,RGB(0,0,0),RGB(183,0,183)}}},
    (button){5,170,40,40,selectcolor,RGB(225,0,0),1,{(shape){'r',1,5,170,40,40,RGB(0,0,0),RGB(225,0,0)}}},
    (button){55,170,40,40,selectcolor,RGB(0,225,0),1,{(shape){'r',1,55,170,40,40,RGB(0,0,0),RGB(0,225,0)}}},
    (button){5,220,40,40,selectcolor,RGB(0,0,255),1,{(shape){'r',1,5,220,40,40,RGB(0,0,0),RGB(0,0,255)}}},
    (button){55,220,40,40,selectcolor,RGB(159,0,177),1,{(shape){'r',1,55,220,40,40,RGB(0,0,0),RGB(159,0,177)}}},
    (button){5,275,40,40,selectcolor,RGB(255,255,255),1,{(shape){'r',1,5,275,40,40,RGB(0,0,0),RGB(255,255,255)}}},
    (button){55,275,40,40,selectcolor,RGB(0,0,0),1,{(shape){'r',1,55,275,40,40,RGB(0,0,0),RGB(0,0,0)}}},
    (button){5,330,40,40,selectcolor,RGB(225,225,0),1,{(shape){'r',1,5,330,40,40,RGB(0,0,0),RGB(255,255,0)}}},
    (button){5,400,40,40,imgselector,1,4,{(shape){'r',0,23,403,4,20,RGB(0,0,0),RGB(0,0,0)},(shape){'l',0,15,423,35,423,RGB(0,0,0),RGB(0,0,0)},(shape){'l',0,15,423,25,435,RGB(0,0,0),RGB(0,0,0)},(shape){'l',0,25,435,35,423,RGB(0,0,0),RGB(0,0,0)}}},
    (button){55,400,40,40,writeimage,1,4,{(shape){'r',0,73,415,4,20,RGB(0,0,0),RGB(0,0,0)},(shape){'l',0,65,415,85,415,RGB(0,0,0),RGB(0,0,0)},(shape){'l',0,65,415,75,405,RGB(0,0,0),RGB(0,0,0)},(shape){'l',0,75,405,85,415,RGB(0,0,0),RGB(0,0,0)}}}
};


int main(){
    button b;
    int i,j,x,y,size,csize;

    initwindow(1180,720,"fjvn");
    line(99,0,99,720);
    setfillstyle(SOLID_FILL,WHITE);
    floodfill(100,0,WHITE);
    setfillstyle(SOLID_FILL,RGB(150,150,150));
    floodfill(0,0,WHITE);
    

   
    size=imagesize(100,0,1180,720);
    void *bitmap=(void*)malloc(size);
    getimage(100,0,1180,720,bitmap);
    bmpp=&bitmap;

    csize=imagesize(0,0,99,720);
    void *cmap=(void*)malloc(csize);
    
    for(i=0;i<numbuttons;i++){
        drawbutton(buttons[i]);
        addEvent(buttons[i]);
    }

    getimage(0,0,99,720,cmap);
    
    orderEvents();

    
    clearmouseclick(WM_LBUTTONDOWN);
    clearmouseclick(WM_LBUTTONUP);
    while(1){
        if(ismouseclick(WM_LBUTTONDOWN)){
            getmouseclick(WM_LBUTTONDOWN,x,y);
            if(x<100){
                findevent(x,y,cmap);
                clearmouseclick(WM_LBUTTONDOWN);
                clearmouseclick(WM_LBUTTONUP);
            }
            else{
                clearmouseclick(WM_LBUTTONUP);
                drawShape(x,y,bitmap);
            }
        }
    }
    
}