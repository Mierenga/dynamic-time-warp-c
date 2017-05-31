#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define VERY_BIG  (0xffff)

/* dtw.c */
/* VERSION 2.0 Andrew Slater, 20/8/1999 */
/* Latest changes 3/2006 by John Coleman */

/* DESCRIPTION */
/* Compute a distance matrix on 2 multi-parameter vectors from 2 utterances,
   and perform dynamic time warping on the distance matrix */

/* INPUT: */
/* Two ASCII parameter files of the format:

filex:

X1a   X1b ... X1n
X2a   X2b ... X2n
...

filey:

Y1a   Y1b ... Y1n
Y2a   Y2b ... Y2n
...

where a, b ... n are parameters (e.g. f0, tongue-tip x co-ordinate)
1 ... n is a time series
X and Y are 2 utterances

Distance is calculated as:

Dist[x1][y1] = (X1a - Y1a)^2 + (X1b - Y1b)^2 + ... + (X1n - Y1n)^2, etc.

 */

/* OUTPUTS: */
/* output file: best alignment of the 2 parameter files */
/* glob: sum of global distances, useful as a similarity measure */

typedef unsigned short int uint16;

int main(argc, argv)
    int argc;
    char *argv[];

{

    uint16 **globdist;
    uint16 **Dist;

    uint16 top, mid, bot, cheapest, total;
    uint16 **move;
    uint16 **warp;
    uint16 **temp;

    uint16 I, X, Y, n, i, j, k;
    uint16 xsize = atoi(argv[4]);
    uint16 ysize = atoi(argv[5]);
    uint16 params = atoi(argv[6]);

    uint16 debug; /* debug flag */
    #define DEBUG (1)

    uint16 **x, **y; /*now 2 dimensional*/

    FILE *file1, *file2, *glob, *debug_file, *output_file;

    if (argc < 7 || argc > 8)
    {
        fprintf(stderr,"Usage: dtw infile1 infile2 outfile xsize ysize num_params [debug_file]\n");
        exit(1);
    }

    if (argc == 8)
    {
        /* open debug file */

        if ((debug_file = fopen(argv[7],"wb")) == NULL)
        {
            fprintf(stderr,"Cannot open debug file\n");
            exit(1);
        }

        debug = 1;
    }

    /* open x-parameter file */

    if ((file1=fopen(argv[1],"rb")) == NULL)
    {
        fprintf(stderr,"File %s cannot be opened\n",argv[1]);
        exit(1);
    }

    /* open y-parameter file */

    if ((file2=fopen(argv[2],"rb")) == NULL)
    {
        fprintf(stderr,"File %s cannot be opened\n",argv[2]);
        exit(1);
    }

#if DEBUG
    if (debug == 1) 
        fprintf(debug_file,"xsize %d ysize %d params %d\n",xsize,ysize,params);
#endif

    /* allocate memory for x and y matrices */

    if ((x = malloc(xsize * sizeof(uint16*))) == NULL)
        fprintf(stderr,"Memory allocation error (x)\n");

    for (i=0; i < xsize; i++)
        if ((x[i] = malloc(params * sizeof(uint16))) == NULL)
            fprintf(stderr,"Memory allocation error (x)\n");

    if ((y = malloc(ysize * sizeof(uint16*))) == NULL)
        fprintf(stderr,"Memory allocation error (y)\n");

    for (i=0; i < ysize; i++)
        if ((y[i] = malloc(params * sizeof(uint16))) == NULL)
            fprintf(stderr,"Memory allocation error (y)\n");

    /* allocate memory for Dist */

    if ((Dist = malloc(xsize * sizeof(uint16*))) == NULL)
        fprintf(stderr,"Memory allocation error (Dist)\n");

    for (i=0; i < xsize; i++)
        if ((Dist[i] = malloc(ysize * sizeof(uint16))) == NULL)
            fprintf(stderr,"Memory allocation error (Dist)\n");

    /* allocate memory for globdist */

    if ((globdist = malloc(xsize * sizeof(uint16*))) == NULL)
        fprintf(stderr,"Memory allocation error (globdist)\n");

    for (i=0; i < xsize; i++)
        if ((globdist[i] = malloc(ysize * sizeof(uint16))) == NULL)
            fprintf(stderr,"Memory allocation error (globdist)\n");

    /* allocate memory for move */

    if ((move = malloc(xsize * sizeof(uint16 *))) == NULL)
        fprintf(stderr,"Memory allocation error (move)\n");

    for (i=0; i < xsize; i++)
        if ((move[i] = malloc(ysize * sizeof(uint16))) == NULL)
            fprintf(stderr,"Memory allocation error (move)\n");

    /* allocate memory for temp */

    if ((temp = malloc(xsize * 2 * sizeof(uint16 *))) == NULL)
        fprintf(stderr,"Memory allocation error (temp)\n");

    for (i=0; i < xsize*2; i++)
        if ((temp[i] = malloc(2 * sizeof(uint16))) == NULL)
            fprintf(stderr,"Memory allocation error (temp)\n");

    /* allocate memory for warp */

    if ((warp = malloc(xsize * 2 * sizeof(uint16 *))) == NULL)
        fprintf(stderr,"Memory allocation error (warp)\n");

    for (i=0; i < xsize*2; i++)
        if ((warp[i] = malloc(2 * sizeof(uint16))) == NULL)
            fprintf(stderr,"Memory allocation error (warp)\n");

    fprintf(stdout,"Reading input files\n");

    /*read x parameter in x[]*/

    for (i=0; i < xsize; i++)
    {
        for (k=0; k < params; k++)
        {
            if (feof(file1))
            {
                fprintf(stderr,"Premature EOF in %s\n",argv[1]);
                exit(1);
            }

            fscanf(file1,"%hu ",&x[i][k]);

#if DEBUG
            if (debug == 1)
                fprintf(debug_file,"float_x[%d %d] = %d\n",i,k,x[i][k]);
#endif
        }
    }

    /*read y parameter in y[]*/

    for (i=0; i < ysize; i++)
    {
        for (k=0; k < params; k++)
        {

            if (feof(file2))
            {
                fprintf(stderr,"Premature EOF in %s\n",argv[2]);
                exit(1);
            }

            fscanf(file2,"%hu ",&y[i][k]);
#if DEBUG
            if (debug == 1)
                fprintf(debug_file,"float_y[%d %d] = %d\n",i,k,y[i][k]);
#endif

        }
    }

    fprintf(stdout,"Computing distance matrix ...\n");

    /*Compute distance matrix*/

    for(i=0;i<xsize;i++)
    {
        for(j=0;j<ysize;j++)
        {
            total = 0;
            for (k=0;k<params;k++)
            {
                total = total + ((x[i][k] - y[j][k]) * (x[i][k] - y[j][k]));
                printf("total: %d\n", total);
            }

            Dist[i][j] = total;

#if DEBUG
            if (debug == 1)
                fprintf(debug_file,"Dist: %d %d %d %d\n",i,j,total,Dist[i][j]);
#endif
        }
    }

    fprintf(stdout,"Warping in progress ...\n");

    /*% for first frame, only possible match is at (0,0)*/

    globdist[0][0] = Dist[0][0];
    for (j=1; j<xsize; j++)
        globdist[j][0] = VERY_BIG;

    globdist[0][1] = VERY_BIG;
    globdist[1][1] = globdist[0][0] + Dist[1][1];
    move[1][1] = 2;

    for(j=2;j<xsize;j++)
        globdist[j][1] = VERY_BIG;

    for (i=2;i<ysize;i++) 
    {
        globdist[0][i] = VERY_BIG;
        globdist[1][i] = globdist[0][i-1] + Dist[1][i];

#if DEBUG
        if (debug == 1)
            fprintf(debug_file,"globdist[2][%d] = %d\n",i,globdist[2][i]);
#endif

        for (j=2;j<xsize;j++)
        {
            top = globdist[j-1][i-2] + Dist[j][i-1] + Dist[j][i];
            mid = globdist[j-1][i-1] + Dist[j][i];
            bot = globdist[j-2][i-1] + Dist[j-1][i] + Dist[j][i];
            if ((top < mid) && (top < bot))
            {
                cheapest = top;
                I = 1;
            }
            else if (mid < bot)
            {
                cheapest = mid;
                I = 2;
            }
            else
            {
                cheapest = bot;
                I = 3;
            }

            /*if all costs are equal, pick middle path*/
            if ((top == mid) && (mid == bot))
                I = 2;

            globdist[j][i] = cheapest;
            printf("i: %d, j: %d\n", i, j);

            move[j][i] = I;
#if DEBUG
            if (debug == 1)
            {
                fprintf(debug_file,"globdist[%d][%d] = %d\n",j,i,globdist[j][i]);
                fprintf(debug_file,"move j:%d:i:%d=%d\n",j,i,move[j][i]);
            }
#endif
        }
    }

#if DEBUG
    if (debug == 1)
    {
        for (j=0; j<xsize; j++)
        {
            for (i=0; i<ysize; i++) 
            {
                fprintf(debug_file,"[%d %d] globdist: %d    move: %d    \n",j,i,globdist[j][i],move[j][i]);
            }
        }
    }
#endif


    X = ysize-1; Y = xsize-1; n=0;
    warp[n][0] = X; warp[n][1] = Y;


    //while (X > 0 && Y > 0)
///TODO
    while (X > 1 && Y > 1)
    {
        n=n+1;

        if (n>ysize *2) 
        {
            fprintf (stderr,"Warning: warp matrix too large!");
            exit(1);
        } 

#if DEBUG
        if (debug == 1)
            fprintf(debug_file,"Move %d %d %d\n", Y, X, move[Y][X]);
#endif

        if (move[Y] [X] == 1 )
        {
            warp[n][0] = X-1; warp[n][1] = Y;
            n=n+1;
            X=X-2; Y = Y-1;
        }
        else if (move[Y] [X] == 2)
        {
            X=X-1; Y = Y-1;
        }
        else if (move[Y] [X] == 3 )
        {
            warp[n] [0] = X;
            warp[n] [1] = Y-1; 
            n=n+1;
            X=X-1; Y = Y-2;
        }
        else 
        {
            
            fprintf(stderr,"Error: move not defined for X = %d Y = %d\n",X,Y); 
            fprintf(stderr,"\tMove was: %d\n", move[Y][X]); 
            fprintf(stderr,"Path not available!"); 
        }
        warp[n] [0] =X;
        warp[n] [1] =Y;

    }


    /*flip warp*/
    for (i=0;i<=n;i++) 
    {
        temp[i][0] = warp[n-i][0];
        temp[i][1] = warp[n-i][1];

    }

    for (i=0;i<=n;i++)
    {
        warp[i][0] = temp[i][0];
        warp[i][1] = temp[i][1];

    }

    fprintf(stdout,"Warping complete. Writing output file.\n");

    /* open output file */
    if ((output_file=fopen(argv[3],"wb")) == NULL)
    {
        fprintf(stderr,"File %s cannot be opened\n",argv[3]);
        exit(1);
    }

    /*print warped trajectory to stdout*/
    for (i=0;i<=n;i++)
        fprintf(output_file,"%d %d\n",warp[i][0]+1,warp[i][1]+1);

    fclose(output_file);

    /* print global distance to globfile*/     

    if ((glob=fopen("glob","w"))==NULL)
        fprintf(stderr,"Cannot open file glob\n");

    fprintf(glob,"%d\n",globdist[xsize-1][ysize-1]);
    fclose(glob);

    fprintf(stdout,"%s     %s     %d\n",argv[1],argv[2],globdist[xsize-1][ysize-1]);


    return 0;
}




