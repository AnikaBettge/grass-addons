/****************************************************************
 * MODULE:     v.path.obstacles
 *
 * AUTHOR(S):  Maximilian Maldacker
 *  
 *
 * COPYRIGHT:  (C) 2002-2005 by the GRASS Development Team
 *
 *             This program is free software under the
 *             GNU General Public License (>=v2).
 *             Read the file COPYING that comes with GRASS
 *             for details.
 *
 ****************************************************************/

#include "rotation_tree.h"


void add_rightmost(struct Point* p, struct Point* q)
{
	struct Point * right;
	
	p->left_brother = NULL;
	p->right_brother = NULL;
	
	if ( q->rightmost_son == NULL )
	{
		q->rightmost_son = p;
	}
	else
	{
		right = q->rightmost_son;
		
		right->right_brother = p;
		p->left_brother = right;
		
		q->rightmost_son = p;
	}
	
	p->father = q;
	

}

void add_leftof(struct Point* p, struct Point* q)
{
	struct Point * left;

	if ( q->left_brother == NULL )
	{
		p->left_brother = NULL;
		q->left_brother = p;
		p->right_brother = q;
	}
	else
	{
		left = q->left_brother;
		p->left_brother = left;
		left->right_brother = p;
		p->right_brother = q;
		q->left_brother = p;
	}

	
	p->father = q->father;
}

void remove_point(struct Point* p)
{
	struct Point * f = p->father;
	struct Point * l = p->left_brother;
	struct Point * r = p->right_brother;

	if ( l != NULL )
		l->right_brother = r;
	if ( r != NULL )
		r->left_brother = l;
	
	if ( f->rightmost_son == p )
		f->rightmost_son = NULL;
		
	p->father = NULL;
	p->left_brother = NULL;
	p->right_brother = NULL;

}

struct Point* right_brother(struct Point* p)
{
	return p->right_brother;
}

struct Point* left_brother(struct Point* p)
{
	return p->left_brother;
}

struct Point* father( struct Point* p)
{
	return p->father;
}

struct Point* rightmost_son( struct Point * p )
{
	return p->rightmost_son;
}

struct Line * segment1( struct Point * p )
{
	return p->line1;
}


struct Line * segment2( struct Point * p )
{
	return p->line2;
}

struct Point * other1( struct Point * p )
{	
	if ( p->line1 == NULL)
		return NULL;

	if ( p->line1->p1 == p )
		return p->line1->p2;
	else
		return p->line1->p1;
}

struct Point * other2( struct Point * p )
{	
	if( p->line2 == NULL )
		return NULL;

	if ( p->line2->p1 == p )
		return p->line2->p2;
	else
		return p->line2->p1;
}

double segment_sqdistance( struct Point * q, struct Line * e )
{
	
	double e2e1x =  e->p1->x - e->p2->x ;
	double e2e1y = e->p1->y - e->p2->y;
	
	double qe1x = q->x - e->p1->x;
	double qe1y = q->y - e->p1->y;
	
	double qe2x = q->x - e->p2->x;
	double qe2y = q->y - e->p2->y;
	
	double s = e2e1x * qe2x + e2e1y * qe2y;
	double t;
	
	if ( s <= 0 )
		return qe2x * qe2x + qe2y * qe2y;
		
	t = e2e1x * e2e1x + e2e1y * e2e1y;
	
	if ( s >= t )
		return qe1x * qe1x + qe1y * qe1y;
	
	 return qe2x * qe2x + qe2y * qe2y - s * s / t ;

}


int before( struct Point * p, struct Point * q, struct Line * e )
{
	/* true if q lies nearer to p than segment e*/
	
	/* first determine the square distance between p and e */
	
	if ( e == NULL )
		return 1;
	
	double e_distance = segment_sqdistance(p, e);
	double pqx =  q->x - p->x;
	double pqy = q->y - p->y;
	double pq_distance = pqx*pqx + pqy*pqy;

	return pq_distance < e_distance ;
}

/** dot product between qp and qr */
double dot( struct Point * p, struct Point * q, struct Point * r )
{
	double qp_x = p->x - q->x;
	double qp_y = p->y - q->x;
	double qr_x = r->x - q->x;
	double qr_y = r->y - q->y;
	return	(qp_x * qr_x + qp_y * qr_y);
}

double distance( struct Point * p, struct Point * q )
{
	double dx = p->x - q->x;
	double dy = p->y - q->y;
	return sqrt( dx*dx + dy*dy);
}


