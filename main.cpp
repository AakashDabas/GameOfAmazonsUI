#include<iostream>
#include<opencv2/opencv.hpp>
#include<opencv2/core/core.hpp>
#include<opencv2/highgui/highgui.hpp>
#include<opencv2/imgproc/imgproc.hpp>
#include<sys/time.h>

using namespace std;
using namespace cv;

#define pix(a,x,y) a.at<Vec3b>(Point(x,y))
int shwT=1;

struct comSize{
    int xL=10000,xU=0,yL=10000,yU=0;
}cPt;

void cngSize(Mat in,Mat &out,float s)
{
    Mat tmp(Size(in.cols/s,in.rows/s),in.type());
    for(int i=0;i<tmp.cols;i++)
        for(int j=0;j<tmp.rows;j++)
        {
            int r,g,b;
            r=g=b=0;
            for(int m=0;m<s;m++)
                for(int n=0;n<s;n++)
                {
                    Vec3b p=pix(in,i*s+m,j*s+n);
                    r+=p.val[2];
                    g+=p.val[1];
                    b+=p.val[0];
                }
            r/=s*s;
            g/=s*s;
            b/=s*s;
            Vec3b p;
            p.val[0]=b;
            p.val[1]=g;
            p.val[2]=r;
            pix(tmp,i,j)=p;
        }
    out.release();
    out=tmp;
}

void extract(Mat &in,int x,int y,int &nPix,int act=0,unsigned char pixVal1=250,unsigned char pixVal2=240)
{
    for(int i=-1;i<=1;i++)
        for(int j=-1;j<=1;j++)
        {
            if(i+x>=0&&i+x<in.cols&&j+y>=0&&j+y<in.rows){
                Vec3b p=pix(in,i+x,j+y);
                if(p.val[0]==pixVal1)
                {
                    if(i+x>cPt.xU)
                        cPt.xU=i+x;
                    else if(i+x<cPt.xL)
                        cPt.xL=i+x;
                    if(j+y>cPt.yU)
                        cPt.yU=j+y;
                    else if(j+y<cPt.yL)
                        cPt.yL=j+y;
                    if(act==1)
                    {
                        p.val[0]=p.val[2]=150;
                        p.val[1]=50;
                    }
                    else if(act==2)
                    {
                        p.val[0]=p.val[1]=200;
                        p.val[2]=50;
                    }
                    else if(act==0)
                        p.val[0]=p.val[1]=p.val[2]=pixVal2;
                    nPix++;
                    pix(in,i+x,j+y)=p;
                    extract(in,i+x,j+y,nPix,act,pixVal1,pixVal2);
                }
            }
        }
}

bool chkTarget(Mat in)
{
/*
 * Count of dull pixels->Possibly shadow
*/
    int countB=0,thB=50;
    for(int i=0;i<in.cols;i++)
        for(int j=0;j<in.rows;j++)
        {
            if(countB/(float)(in.cols*in.rows)>0.05)
                return false;
            Vec3b p=pix(in,i,j);
            if(p.val[0]<thB&&p.val[1]<thB&&p.val[2]<thB)
                countB++;
        }
    return true;
}

void getTargets(char *loc,Mat &out)
{
    Mat in=imread(loc,CV_LOAD_IMAGE_COLOR);
//    Mat t=imread(loc);
    Mat sDown,inBk=in.clone();
    int uS=4;
    cngSize(in,in,2);
    GaussianBlur(in,in,Size(13,13),0);//To reduce noise
    cngSize(in,sDown,uS);
    Mat sOut(Size(in.cols,in.rows),in.type());
    for(int i=0;i<in.cols;i++)
        for(int j=0;j<in.rows;j++)
        {
            int th=15;
            Vec3b p1=pix(in,i,j);
            Vec3b p2=pix(sDown,i/uS,j/uS);
            Vec3b p;
            if(abs(p1.val[0]-p2.val[0])>th||
               abs(p1.val[1]-p2.val[1])>th||
               abs(p1.val[2]-p2.val[2]>th))
                p.val[0]=p.val[1]=p.val[2]=250;
            else
                p.val[0]=p.val[1]=p.val[2]=0;
            pix(sOut,i,j)=p;
        }
    cngSize(sOut,sOut,2);
    for(int i=0;i<sOut.cols;i++)
        for(int j=0;j<sOut.rows;j++)
        {
            Vec3b p=pix(sOut,i,j);
            if(p.val[0]>20)
                p.val[0]=p.val[1]=p.val[2]=250;
            else
                p.val[0]=p.val[1]=p.val[2]=50;
            pix(sOut,i,j)=p;
        }

    for(int i=0,tgtN=0;i<sOut.cols;i++)
        for(int j=0;j<sOut.rows;j++)
        {
            Vec3b p=pix(sOut,i,j);
            if(p.val[0]==250)
            {
                int nPix=0;
                cPt.xL=cPt.yL=10000;
                cPt.xU=cPt.yU=0;
                extract(sOut,i,j,nPix);
                float w=cPt.xU-cPt.xL,h=cPt.yU-cPt.yL;
                if(nPix<20)
                    extract(sOut,i,j,nPix,1,240,250);//Changes here
                else if(nPix>500||w>50||h>50||!h||w/h>1.8||!w||h/w>1.8)
                    extract(sOut,i,j,nPix,2,240,250);
                else
                {
//                    cout<<cPt.xL<<"_"<<cPt.xU<<"_"<<w<<"_"<<h<<endl;
                    Mat trgt(Size(w*4,h*4),in.type());
                    cPt.xL*=4;
                    cPt.xU*=4;
                    cPt.yL*=4;
                    cPt.yU*=4;
                    for(int m=cPt.xL;m<cPt.xU;m++)
                        for(int n=cPt.yL;n<cPt.yU;n++)
                        {
                            pix(trgt,m-cPt.xL,n-cPt.yL)=pix(inBk,m,n);
                        }
                       tgtN++;
                    if(chkTarget(trgt)&&shwT==tgtN)
                        imshow("target",trgt);
                    else
                        extract(sOut,i,j,nPix,1,240,250);
                }
            }
        }
    out=sOut;
}

int main()
{
    Mat out;
    char loc[]="in1.jpg";
////    timeval start,end;
////    gettimeofday(&start,NULL);
////    for(int i=0;i<50;i++)
////    getTarget(loc,out);
////    gettimeofday(&end,NULL);
////    double delta = ((end.tv_sec  - start.tv_sec) * 1000000u + end.tv_usec - start.tv_usec) / 1.e6;
////    cout<<delta;
////    cngSize(out,out,4);
////    imshow("output",out);
////    waitKey(0);
    while(1)
    {
        getTargets(loc,out);
        imshow("output",out);
        Mat in=imread(loc);
        cngSize(in,in,4);
        imshow("input",in);
        int keyIn=waitKey(1);
        if(keyIn=='w')
            loc[2]++,shwT=1;
        else if(keyIn=='s')
            loc[2]--,shwT=1;
        else if(keyIn==27||keyIn==13)
            break;
        else if(keyIn=='d'||keyIn=='D')
            shwT++;
        else if(keyIn=='a'||keyIn=='A'&&shwT>0)
            shwT--;
    }
    return 0;
}
