#include "local_proto.h"

/* 
 * global function declaration 
 */

void build_weight_vect(int nrows, int ncols, int ncriteria, struct Option *weight, double *weight_vect);

void build_dominance_matrix(int nrows, int ncols, int ncriteria, double *weight_vect, double **concordance_mat, double **discordance_mat, double ***decision_vol);


/*
 * function definitions 
 */

void build_weight_vect(int nrows, int ncols, int ncriteria,struct Option *weight, double *weight_vect)
{
	
	int i,nweight=0;
	double weight_sum=0;
	
	while (weight->answers[nweight]!=NULL)
    	{
    		nweight++;
    	}

    	 
	if(nweight!=ncriteria)
		G_fatal_error(_("criteria number  and weight number are different"));

	
	for(i=0;i<nweight;i++)
	{
		weight_vect[i]=(atof(weight->answers[i]));	/*transfer  weight value  in double  array*/
		weight_sum=weight_sum+weight_vect[i];		/*calculate sum weight*/
	}
		
	for(i=0;i<nweight;i++)
		{
		weight_vect[i]=weight_vect[i]/weight_sum;	/*normalize vector weight*/

		}

}


void build_dominance_matrix(int nrows, int ncols, int ncriteria, double *weight_vect, double **concordance_mat, double **discordance_mat, double ***decision_vol)
{		
	int row1,col1,row2,col2;
	int i,j,k,cont;
	double row_sum_conc, col_sum_conc,row_sum_disc, col_sum_disc;
	
	k=0;	/* make pairwise comparation and build concordance/discordance matrix*/
	for (row1 = 0; row1 < nrows; row1++) 
		{
		G_percent(row1, nrows, 2);
		for (col1 = 0; col1 < ncols; col1++)
			{
			j=0;
			for (row2 = 0; row2 < nrows; row2++) 
				{
				for (col2 = 0; col2 < ncols; col2++)
					{
					for(i=0;i<ncriteria;i++)
						{
						if(decision_vol[row1][col1][i]>=decision_vol[row2][col2][i])
							{concordance_mat[k][j]=concordance_mat[k][j]+weight_vect[i];}
							
						if((decision_vol[row1][col1][i]-decision_vol[row2][col2][i])>concordance_mat[k][j])
							{discordance_mat[k][j]=(decision_vol[row2][col2][i]-decision_vol[row1][col1][i]);}
						}
					j++;/* increase rows index up to nrows*ncols*/
					}
				}
				k++; /* increase columns index up to nrows*ncols*/
			}
		}

		/*calculate concordance and discordance index and storage in decision_vol*/
	cont=0;		/*variabile progressiva per riga/colonna della concordance_map*/
	for(row1=0;row1<nrows;row1++)
		{
		G_percent(row1, nrows, 2);
		for(col1=0;col1<ncols;col1++)
			{
			row_sum_conc=col_sum_conc=row_sum_disc=col_sum_disc=0;
			 /* da un valore incrementale ad ogni ciclo per progredire nella concordance_mat */
			for(i=0;i<nrows*ncols;i++)
				{
				/*concordance index */
				row_sum_conc=row_sum_conc+concordance_mat[cont][i];
				col_sum_conc=row_sum_conc+concordance_mat[i][cont];
				/*discordance index */
				row_sum_disc=row_sum_disc+discordance_mat[cont][i];
				col_sum_disc=row_sum_disc+discordance_mat[i][cont];
				}
			cont++;
			decision_vol[row1][col1][ncriteria]=row_sum_conc-col_sum_conc;/*fill matrix with concordance index for each DCELL*/
			decision_vol[row1][col1][ncriteria+1]=row_sum_disc-col_sum_disc;/*fill matrix with discordance index for each DCELL*/
			}
		}
}

