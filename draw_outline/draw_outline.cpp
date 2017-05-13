#include"draw_outline.h"
using namespace std;
struct stack
 {
	 CvPoint pt;
	 struct stack *link;
 };struct stack *start=NULL;

 void stack_push(CvPoint p)
 {
	 struct stack *newelt;
     newelt=(struct stack *)malloc(sizeof(struct stack));
	 newelt->pt = p;
     newelt->link=start;
     start=newelt;
 }

 CvPoint stack_pop()
 {
	 CvPoint pt;
	 struct stack *temp;
	 if(start!=NULL)
	 {
		 pt = start->pt;
		 temp = start;
		 start = start->link;
		 free(temp);
	 }
	 else
	 {
		 pt.x = -1;
		 pt.y = -1;
	 }
	 return(pt);
 }
#ifdef __cplusplus
extern "C"{
#endif
	__declspec(dllexport)int draw_outline(unsigned char* origimgPtr, int rows, int cols, int** contour_pt_arr, int** grayvalues)
{
	cv::Mat orig(rows, cols, CV_8UC3, origimgPtr);
	IplImage* original = new IplImage(orig);
	//Declare the variables
	IplImage *original_gray, *edgeimage, *allone, *label_image,*original_HSV;
	IplImage *vertedge, *horzedge, *diag45edge, *diag135edge;
	CvScalar trans;
	CvScalar white = cvScalar(255,255,255,0);
	CvScalar black = cvScalar(0,0,0,0);
	CvScalar ext_color = cvScalar(255,0,0,0);
	CvScalar hole_color = cvScalar(0,255,0,0);
	CvMemStorage *storage;
	CvMemStorage *storage_connseq;
	CvSeq *connseq, *trav_popu;
	CvPoint p;
	CvScalar s = cvScalar(0,0,0,0);
	int found_flag=0, seq_ind = 0;
	int count = 0, count1 = 0;
	int flagput = 0, vectorind = 0;
	
	//Processing
	
	//1. Resizing
	//IplImage *original_big1 = cvLoadImage("C:\\Users\\Saumya\\Desktop\\car1.jpg", CV_LOAD_IMAGE_COLOR);

	//original = cvCreateImage(cvSize(250,250),IPL_DEPTH_8U,3);
	//cvResize(original_big, original);
	//cvShowImage("Edge Image", original);cvWaitKey(0);

	int h = original->height;
	int w = original->width;
	
	//2. Rgb to Grayscale 
	original_gray = cvCreateImage(cvSize(w, h),IPL_DEPTH_8U,1);
	original_HSV = cvCreateImage(cvSize(w, h),IPL_DEPTH_8U,3);
	cvCvtColor(original, original_HSV, CV_RGB2HSV);
	for(int i=0;i<h;i++)
	{
		for(int j=0;j<w;j++)
		{
			s = cvGet2D(original_HSV,i,j);
			s.val[0] = s.val[2];
			cvSet2D(original_gray, i, j, s);
		}
	}
	//cvShowImage("hsv",original_gray);cvWaitKey(0);
	//cvEqualizeHist(original_gray, original_gray);
	

	//3. Consolidation of 4 edges
	float dataVert[] = {1, 0, -1, 2, 0, -2, 1, 0, -1};
	CvMat vert = cvMat(3, 3, CV_32FC1, dataVert);
	vertedge = cvCreateImage(cvSize(w, h),IPL_DEPTH_8U,1);
	cvFilter2D(original_gray, vertedge, &vert);
	//cvShowImage("vert", vertedge);cvWaitKey(0);

	float dataHorz[] = {1, 2, 1, 0, 0, 0, -1, -2, -1};
	CvMat horz = cvMat(3, 3, CV_32FC1, dataHorz);
	horzedge = cvCreateImage(cvSize(w, h),IPL_DEPTH_8U,1);
	cvFilter2D(original_gray, horzedge, &horz);
	//cvShowImage("horz", horzedge);cvWaitKey(0);

	float dataDiag45[] = {2, 1, 0, 1, 0, -1, 0, -1, -2};
	CvMat diag45 = cvMat(3, 3, CV_32FC1, dataDiag45);
	diag45edge = cvCreateImage(cvSize(w, h),IPL_DEPTH_8U,1);
	cvFilter2D(original_gray, diag45edge, &diag45);
	//cvShowImage("diag45", diag45edge);cvWaitKey(0);

	float dataDiag135[] = {0, 1, 2, -1, 0, 1, -2, -1, 0};
	CvMat diag135 = cvMat(3, 3, CV_32FC1, dataDiag135);
	diag135edge = cvCreateImage(cvSize(w, h),IPL_DEPTH_8U,1);
	cvFilter2D(original_gray, diag135edge, &diag135);
	//cvShowImage("diag135", diag135edge);cvWaitKey(0);

	edgeimage = cvCreateImage(cvSize(w, h),IPL_DEPTH_8U,1);
	cvAdd(vertedge, horzedge, edgeimage);
	cvAdd(diag45edge, edgeimage, edgeimage);
	cvAdd(diag135edge, edgeimage, edgeimage);
	//cvShowImage("total edges", edgeimage); cvWaitKey(0);

	//Clear out the individual edges
	cvReleaseData(vertedge);
	cvReleaseData(horzedge);
	cvReleaseData(diag45edge);
	cvReleaseData(diag135edge);

	//cvThreshold(edgeimage, edgeimage, 128, 255, CV_THRESH_BINARY);
	//------------CONTOURS ONE BY ONE------------//
	
	allone = cvCreateImage(cvSize(w, h),IPL_DEPTH_8U,1);
	cvSetZero(allone);
	cvThreshold(allone, allone, 128, 255, CV_THRESH_BINARY_INV);

	cvSub(allone, edgeimage, edgeimage);
	//cvShowImage("edgeimage_inv", edgeimage);cvWaitKey(0);
	cvSmooth(edgeimage, edgeimage, CV_GAUSSIAN);
	//cvShowImage("smoothened edge", edgeimage); cvWaitKey(0);
	

	//Quantise the image into 5 separate gray levels, send it to build the labelling image (only 1)
	
	label_image = cvCreateImage(cvSize(w,h),IPL_DEPTH_32S,1);
	for(int label_ind = 0; label_ind<1; label_ind++)
	{	
		//----------------------Create the label image---------------------------//
		for(int i=0;i<h;i++)
		{
			for(int j=0;j<w;j++)
			{
				s = cvGet2D(edgeimage,i,j);
				switch(label_ind)
				{
				case 0:
					if(s.val[0]<50)
						s.val[0] = 0;
					else if(s.val[0]>=50 && s.val[0]<100)
						s.val[0] = 50;
					else if(s.val[0]>=100 && s.val[0]<150)
						s.val[0] = 100;
					else if(s.val[0]>=150 && s.val[0]<200)
						s.val[0] = 150;
					else if(s.val[0]>=200 && s.val[0]<255)
						s.val[0] = 200;
					else 
						s.val[0] = -1000;//255;
					break;
				case 1:
					if(s.val[0]>=100 && s.val[0]<150)
						s.val[0] = 110;
					else
						s.val[0] = -1000;//255;
					break;
				case 2:
					if(s.val[0]>=150 && s.val[0]<230)
						s.val[0] = 200;
					else
						s.val[0] = -1000;//255;
					break;
				case 3:
					if(s.val[0]>=180 && s.val[0]<220)
						s.val[0] = 200;
					else
						s.val[0] = -1000;//255;
					break;
				case 4:
					if(s.val[0]>=220 && s.val[0]<256)
						s.val[0] = 255;
					else
						s.val[0] = -1000;//255;
					break;
				}
				cvSet2D(label_image,i,j,s); 
			}
		}
		//cvShowImage("label image", label_image);cvWaitKey(0);
		//----------------------------------------------------------------------//



		//------------Build connected components, sp, grayval (sorting)--------------//
		int label_count = -1000;
		//auto growing arrays - 1. coords1, 2. coords, 3. grayvalue
		vector<int> coord1;
		vector<int> coord2;
		vector<int> grayvals;
		vector<int> labelseq;
		for(int i=0;i<h;i++)
		{
			for(int j=0;j<w;j++)
			{
				s = cvGet2D(label_image,i,j);
				if(s.val[0]>=0)
				{
					label_count--;

					coord1.push_back(i);
					coord2.push_back(j);

					//if(label_count == -1001)
					//{
						grayvals.push_back(s.val[0]);
						labelseq.push_back(label_count);

					//}
					/*else
					{
						for(int insrtind = label_count; insrtind < -1001; insrtind++)
						{
							flagput = 0;
							vectorind = (-1*insrtind-1001);
							if(s.val[0] >= grayvals.at(vectorind-1))
							{
								flagput = 1;
								grayvals.insert(grayvals.begin()+vectorind,s.val[0]);
								labelseq.insert(labelseq.begin()+vectorind,label_count);
								break;
							}
						}
						if(flagput!=1)
						{
							grayvals.insert(grayvals.begin(),s.val[0]);
							labelseq.insert(labelseq.begin(),label_count);
						}
					}*/
					cvFloodFill(label_image, cvPoint(j,i),cvScalar(double(label_count),0,0,0),cvScalarAll(0),cvScalarAll(0),0,8,0);
					//cvFloodFill(label_image, cvPoint(j,i),cvScalar(255,0,0,0),cvScalarAll(0),cvScalarAll(0),0,8,0);//ch
					//cvShowImage("label image", label_image);cvWaitKey(0);//ch
				}
			}
			
		}
		
		//----------------------------------------------------------------------------------------------------//


		//-------------------------------Traverse the connected components----------------------------------//
		int start_flag = 0;
		//int no_comp = 1;
		
		//For each component
		for(int comp=-1001;comp>=label_count;comp--)
		{
			if(start_flag==0)
			{
				start_flag=1;
				storage = cvCreateMemStorage(0);
				storage_connseq = cvCreateMemStorage(0);
								
				connseq = cvCreateSeq(CV_SEQ_ELTYPE_POINT, sizeof(CvSeq),sizeof(CvPoint),storage_connseq); 
				trav_popu = connseq;
			}
			else
			{
				trav_popu->h_next = cvCreateSeq(CV_SEQ_ELTYPE_POINT, sizeof(CvSeq),sizeof(CvPoint),storage_connseq); 
				trav_popu = trav_popu->h_next;
			}

			//Access the components as per the size sorted sequence
				//smaller the grayval - darker the color
			vectorind = -1001-comp;//sync
			int compsearch = labelseq.at(vectorind);
			vectorind = -1001-compsearch;//async
			//1.---have the starting coordinate - start from there
			for(int j=coord1.at(vectorind);j<h;j++)
			{
				for(int i=0;i<w;i++)
				{
					s = cvGet2D(label_image,j,i);
					if(s.val[0] == compsearch)//async search - based on size sorting
					{
						p.y = j;
						p.x = i;
						seq_ind++;
						cvSeqPush(trav_popu,&p);
						//cvSet2D(compos,j,i,cvScalar(0,0,0,0));
						//stack_push(p);
						//found_flag=1;
						//break;
					}
				}
				//if(found_flag ==1)
				//	break;
			}

			/*
			//2.--Track the contour and populate the sequence----//
			if(found_flag)
				{
					found_flag=0;
					do
					{
						//pop the element
						p = stack_pop();
						//put it in the sequence
						seq_ind++;
						cvSeqPush(trav_popu,&p);
						//find its 8-connected neighbours
						for(int j=(p.y)-1;j<=(p.y)+1;j++)
						{
							for(int i=(p.x)-1;i<=(p.x+1);i++)
							{
								if(i<0 || j<0 || i>=w || j>=h)
									continue;
								s = cvGet2D(compos,j,i);
								if(s.val[0]==255)
								{
									if(!(i==(p.x) && j==(p.y)))
									{
										cvSet2D(compos,j,i,cvScalar(0,0,0,0));
										stack_push(cvPoint(i,j));
									}

								}
							}
						}
					}while(start!=NULL);
				}
				*/
			
		}//all components processing
		//----------------------------------------------------------------------------------------------------//



		//if(no_comp == 1)
		//	continue;

		//Through DrawContours - All the points and not just only the dominant points in the contours
		//Modification - feed the point sequence into an int array
		contour_pt_arr[0] = (int*)malloc(seq_ind*2*sizeof(int));
		grayvalues[0] = (int*)malloc(seq_ind*sizeof(int));
		
		/*
		if(label_ind==0)
		{
		colorcontours1 = cvCreateImage(cvSize(w,h),IPL_DEPTH_8U,1);
		cvSetZero(colorcontours1);
		cvThreshold(colorcontours1,colorcontours1,128,255,CV_THRESH_BINARY_INV);
		}
		*/
		
		trav_popu = connseq;
		count = 0;
		count1 = 0;
		while(trav_popu!=NULL && count<(seq_ind*2))
		{
			for( int i=0; i<trav_popu->total; i=i++ )
			{
				p = *((CvPoint*) cvGetSeqElem(trav_popu, i));
				contour_pt_arr[0][count++] = p.x;
				contour_pt_arr[0][count++] = p.y;
				grayvalues[0][count1++] = (int)cvGet2D(edgeimage, p.y, p.x).val[0];
				/*
				//cvSet2D(colorcontours1, p.y, p.x, cvGet2D(edgeimage, p.y, p.x));
				p = *((CvPoint*) cvGetSeqElem(trav_popu, i+1));
				contour_pt_arr[0][count++] = p.x;
				contour_pt_arr[0][count++] = p.y;
				grayvalues[0][count1++] = (int)cvGet2D(edgeimage, p.y, p.x).val[0];
				//cvSet2D(colorcontours1, p.y, p.x, cvGet2D(edgeimage, p.y, p.x));
				p = *((CvPoint*) cvGetSeqElem(trav_popu, i+2));
				contour_pt_arr[0][count++] = p.x;
				contour_pt_arr[0][count++] = p.y;
				grayvalues[0][count1++] = (int)cvGet2D(edgeimage, p.y, p.x).val[0];
				//cvSet2D(colorcontours1, p.y, p.x, cvGet2D(edgeimage, p.y, p.x));
				p = *((CvPoint*) cvGetSeqElem(trav_popu, i+3));
				contour_pt_arr[0][count++] = p.x;
				contour_pt_arr[0][count++] = p.y;
				grayvalues[0][count1++] = (int)cvGet2D(edgeimage, p.y, p.x).val[0];
				//cvSet2D(colorcontours1, p.y, p.x, cvGet2D(edgeimage, p.y, p.x));
				//cvShowImage("DrawYou",colorcontours1);cvWaitKey(1);
				*/
			}
			trav_popu = trav_popu->h_next;
		}
		
		cvClearMemStorage(storage);
		cvClearMemStorage(storage_connseq);

		cvReleaseMemStorage(&storage);
		cvReleaseMemStorage(&storage_connseq);
		
	}// end of all label images
	


	//Clear all the dynamically allocated memory
	//cvReleaseData(original);
	cvReleaseData(original_gray);
	cvReleaseData(edgeimage);
	cvReleaseData(label_image);
	cvReleaseData(allone);
	cvReleaseData(label_image);
	cvReleaseData(original_HSV);

	return(count);
}
#ifdef __cplusplus
}
#endif