#include "ContourMatching.hh"
#include <math.h>
#include <iostream>

const unsigned int logo_trans_before_rot=10000; // TODO: calculate usefull value !!

class LengthSorter
{
public:
  const std::vector <Contours::Contour*>& contours;

  LengthSorter(const std::vector <Contours::Contour*>& contour_list)
    : contours(contour_list) {}

  bool operator() (unsigned int i, unsigned int j)
  {
    return contours[i]->size() > contours[j]->size(); 
  }
};

class MatchSorter
{
public:
  bool operator() (const LogoRepresentation::Match* a, const LogoRepresentation::Match* b)
  {
    return a->score > b->score;
  }
};

LogoRepresentation::LogoRepresentation(Contours* logo_contours,
				       unsigned int max_feature_no,
				       unsigned int max_avg_tolerance,
				       unsigned int reduction_shift,
				       double maximum_angle,
				       double angle_step)
{
  source=logo_contours;
  tolerance=max_avg_tolerance;
  shift=reduction_shift;
  rot_max=maximum_angle;
  rot_step=angle_step;
  total_contour_length=0;

  logo_set_count=source->contours.size();
  logo_set_map.resize(logo_set_count);
  for (unsigned int i=0; i<logo_set_count; i++)
    logo_set_map[i]=i;
  
  if (logo_set_count > max_feature_no) {
    std::sort(logo_set_map.begin(), logo_set_map.end(), LengthSorter(source->contours));
    logo_set_count=max_feature_no;
  }

  double angle=.0;
  rot_max=std::min(359.9, fabs(rot_max));
  rot_step=std::max(rot_step, 0.5);

  do {
    logo_sets.push_back(std::vector <LogoContourData> (logo_set_count));
    for (unsigned int c=0; c<logo_set_count; c++) {
      LogoContourData& data=logo_sets.back()[c];
      data.contour=new Contours::Contour();
      if (angle==0) {
	CenterAndReduce(*(source->contours[logo_set_map[c]]),
			*data.contour,
			shift,
			data.rx,
			data.ry);
	total_contour_length+=source->contours[logo_set_map[c]]->size();
      } else
	RotCenterAndReduce(*(source->contours[logo_set_map[c]]),
			   *data.contour,
			   M_PI*angle / 180.0,
			   logo_trans_before_rot,
			   shift,
			   data.rx,
			   data.ry);
    }
    
    if (angle > 0) {
      angle=-angle;
    } else {
      angle=-angle+rot_step;
    }

  } while (angle <= rot_max);
}

LogoRepresentation::~LogoRepresentation()
{
  for (unsigned int s=0; s<logo_sets.size(); s++)
    for (unsigned int j=0; j<logo_set_count; j++)
      delete logo_sets[s][j].contour;
}

double LogoRepresentation::Score(Contours* image)
{
  unsigned int image_set_count=image -> contours.size();

  if (image_set_count==0 || logo_set_count==0) {
    std::cerr << "Warning: nothing to match..." << std::endl;
    return 0.0;
  }

  // build image set
  image_set.resize(image_set_count);
  for (unsigned int c=0; c<image_set_count; c++) {
    ImageContourData& data=image_set[c];
    data.contour=new Contours::Contour();
    CenterAndReduce(*(image->contours[c]),
		      *data.contour,
		      shift,
		      data.rx,
		      data.ry);
  }

  // calculate 1 to 1 matching scores

  for (unsigned int s=0; s<logo_sets.size(); s++)
    for (unsigned int j=0; j<logo_set_count; j++) {
      logo_sets[s][j].matches.resize(image_set_count);
      for (unsigned int i=0; i<image_set_count; i++)
	logo_sets[s][j].matches[i]=new Match(image_set[i], logo_sets[s][j], tolerance, shift,
					     source->contours[logo_set_map[j]]->size(), image->contours[i]);
    }

  // calculate heuristic n to m matching

  double score=.0;
  unsigned int best_set=0;
  unsigned int best_pivot=0;
  for (unsigned int s=0; s<logo_sets.size(); s++) {
    unsigned int pivot=0;
    double current=N_M_Match(s, pivot);
    if (current > score) {
      score=current;
      best_set=s;
      best_pivot=pivot;
    }
  }

  // starting parameters optained from heuristic
  score=(score/ (double) total_contour_length) / (double) tolerance;
  std::cout << score << std::endl;
  const LogoContourData& result=logo_sets[best_set][best_pivot];
  logo_translation.first=(int)result.matches[result.n_to_n_match_index]->transx;
  logo_translation.second=(int)result.matches[result.n_to_n_match_index]->transy;
  if (best_set==0)
    rot_angle=.0;
  else {
    logo_translation.first+=logo_trans_before_rot;
    logo_translation.second+=logo_trans_before_rot;
    rot_angle=rot_step*(double)((best_set+1)/2);
    if (best_set%2==0)
      rot_angle=-rot_angle;
  }

  mapping.resize(logo_set_count);
  for (unsigned int i=0; i<logo_set_count; i++) {
    mapping[i]=std::pair <Contours::Contour*, Contours::Contour*>
      (source->contours[logo_set_map[i]],
       logo_sets[best_set][i].matches[logo_sets[best_set][i].n_to_n_match_index]->cimg);
  }

  // optimize
  
  // clean up
  for (unsigned int s=0; s<logo_sets.size(); s++)
    for (unsigned int j=0; j<logo_set_count; j++) {
      for (unsigned int i=0; i<image_set_count; i++)
	delete logo_sets[s][j].matches[i];
      logo_sets[s][j].matches.clear();
    }
 
  for (unsigned int j=0; j<image_set_count; j++)
    delete image_set[j].contour;
  image_set.clear();

  return score;
}


double LogoRepresentation::N_M_Match(unsigned int set, unsigned int& pivot)
{
  std::vector <LogoContourData>& data=logo_sets[set];
  for (unsigned int i=0; i<logo_set_count; i++) {
    std::sort(data[i].matches.begin(), data[i].matches.end(), MatchSorter());
    //std::cout << "BEST\t" << data[i].matches[0]->score << std::endl;
  }

  unsigned int image_set_count=data[0].matches.size();

  const unsigned int depth=5;
  const unsigned int counterdepth=1000;
  int mdepth=std::min(image_set_count, depth);
  int ndepth=std::min(image_set_count, counterdepth);

  double bestsum=.0;
  pivot=0;
  unsigned int tmpbest[logo_set_count];

  for (unsigned int base=0; base < logo_set_count; base++)
    for (unsigned int m=0; m < mdepth; m++) {
      
      double sum=data[base].matches[m]->score;
      double tx=data[base].matches[m]->transx;
      double ty=data[base].matches[m]->transy;
      tmpbest[base]=m;

      for (unsigned int counter=0; counter < logo_set_count; counter++)
	if (counter != base) {
	  double best=.0;
	  tmpbest[counter]=0;
	  for (unsigned int n=0; n < ndepth; n++){
	    double current=data[counter].matches[n]->TransScore(tx, ty);
	    if (current > best) {
	      best=current;
	      tmpbest[counter]=n;
	    }
	  }
	  sum+=best;
	}

      if (sum > bestsum) {
	bestsum=sum;
	pivot=base;
	for (unsigned int i=0; i<logo_set_count; i++)
	  data[i].n_to_n_match_index=tmpbest[i];
      }
    }
  
  return bestsum;
}


LogoRepresentation::Match::Match(const ImageContourData& image,
	  const LogoContourData& logo,
	  int tolerance,
	  int shift,
	  unsigned int original_logo_length,
	  Contours::Contour* icimg)
{
  length=original_logo_length;
  cimg=icimg;
  score=(double)tolerance*(double)length;
  score-=L1Dist(*logo.contour, *image.contour, logo.rx, logo.ry, image.rx, image.ry, shift, transx, transy);
  if (score < 0.0)
    score=.0;
}

double LogoRepresentation::Match::TransScore(double tx, double ty)
{
  return std::max(.0, score - 0.5*((double)length*(fabs(tx-transx)+fabs(ty-transy))));
}
