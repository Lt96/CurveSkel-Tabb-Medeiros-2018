#include "TabbMedeiros.hpp"

#include <sys/stat.h>
#include <sys/time.h>
#include <chrono>
#include <opencv2/core/core.hpp>
#include <opencv2/core/mat.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/highgui/highgui_c.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>
#include <chrono>

#include "SubSteps.hpp"
#include "Includes.hpp"
#include "ReconstructionStructure.hpp"
#include "SkelGraph.hpp"
#include "ConversionFunctions.hpp"

using std::list;
using std::cout;
using std::endl;
using std::string;


int main(int argc, char **argv)
{
	/// program has 1 required and 2 optional argument
	// mandatory string -- directory with the BB.txt and 0.txt files
	// mandatory bool -- read in all of the connected components (1), or only the largest one (0)
	// optional 1 --  threshold for spurious curve segment classification.  if not specified, it is set to 1e-12.
	// optional 2 -- flag to indicate that the model needs to be converted from a directory of images to the format in 0.txt and BB.txt.
	// If the flag is 0, then the conversion is from the ImageStack representation.
	// If the flag is 1, then the conversion is from the xyz representation.
	// If conversion is needed, then the threshold also needs to be specified.

	double threshold = 1e-12;
	bool need_to_convert = false;
	bool xyz_rep = false;
	bool read_all_connected_components = true;
	string directory = "";

	if (argc >= 3){
		directory = argv[1];



		int flag_value = FromString<int>(argv[2]);

		if (flag_value != 0 && flag_value != 1){
			cout << "Mandatory read all connected components flag not 0 or 1, quitting" << endl;
			exit(1);
		}

		read_all_connected_components = flag_value;

	}	else {
		cout << "Not enough arguments.  prog_name directory threshold." << endl;
		exit(1);
	}

	if (argc >= 4){
		threshold = FromString<double>(argv[3]);
	}

	EnsureDirHasTrailingBackslash(directory);

	if (argc == 5){
		need_to_convert = true;
		int flag_value = FromString<int>(argv[4]);

		if (flag_value != 0 && flag_value != 1){
			cout << "Optional conversion flag not 0 or 1, quitting" << endl;
			exit(1);
		}

		if (flag_value == 1){
			xyz_rep = true;
		}
	}

	if (need_to_convert){
		xyz_rep == false ? ReadIndividualImages(directory) : ReadXYZ(directory);
	}

	// use the need to convert flag, b/c we won't write ID stacks for all of the components.
	TabbMedeiros( L2_induction, directory, threshold, read_all_connected_components, need_to_convert);


	if (need_to_convert){
		// we take care of the write within the function.
		if (xyz_rep == false) {WriteIndividualImages(directory);}
	}

	cout << "Before final return" << endl;


	return(0);
}

void TabbMedeiros(DISTANCE dist_type, string supplied_write_directory, double threshold, bool read_all_connected_components, bool record_xyz_rep){

	string supplied_BBfile = supplied_write_directory + "BB.txt";
	string supplied_object_file = supplied_write_directory + "0.txt";

	ifstream in_test;
	in_test.open(supplied_BBfile.c_str());

	if (!in_test){
		cout << "You forgot the BB file or the path is wrong" << endl << supplied_BBfile << endl;;
		exit(1);
	}
	in_test.close();

	in_test.open(supplied_write_directory.c_str());
	if (!in_test){
		cout << "You forgot the object file or the path is wrong" << endl << supplied_object_file << endl;;
		exit(1);
	}
	in_test.close();

	string source_file;
	float division = 100.0;

	string write_directory ="";

	vector<double> pA(3, 0);
	vector<double> pB(3, 1000);

	ifstream in;
	vector<vector<double> > BB;
	string filename;

	string object_file;

	bool* frontier = 0;
	double* bfs_label_outside_to_inside_double = 0;
	double* bfs_label_outside_to_inside_inverse_double = 0;
	int_type_t big_big_number = 0;
	vector<int_type_t> local_maxima_sparse_rep_int;
	vector<double> local_maxima_sparse_rep_double;
	int_type_t greatest_OtI_distance_double = 0;
	int_type_t seed_node_index = 0;
	int_type_t best_value_int = 0;
	double best_value_double = 0;
	vector<int_type_t> temp_vector;
	double* bfs_label_node_to_node_double = 0;
	double greatest_mod_distance_double = 0;
	bool* already_explored = 0;
	bool* this_voxel_already_treated = 0;
	int loop_counter = 0;
	int_type_t local_tip;
	vector<int_type_t> pre_skeleton_nodes;
	vector<vector<int_type_t> > multiple_paths_from_same_start;
	double local_max_distance_double = 0;
	double* bfs_from_tip_double =0 ;
	bool* pre_skeleton_bool = 0;
	bool* stop_lines = 0;
	vector<pair<int_type_t, double> > pre_skel_distance_double;
	bool passed_spurious_tests;
	bool* stop_lines_copy = 0;
	vector<vector<int_type_t> > pending_paths;
	bool walks_through_another_stop_set0;
	bool walks_through_another_stop_set1;


	vector<int_type_t> linked_path;
	vector< vector<int_type_t> > keep_components;

	vector<int_type_t> connected_components_info;
	int_type_t biggest_cc;
	int_type_t path_counter;
	int_type_t last_path_added;
	pair<int_type_t, int_type_t> biggest_cc_total_cc_pair;
	int_type_t current_path_id;

	vector<vector<int_type_t> > paths;
	vector<int_type_t> current_path;
	vector<vector<double> > path_diameters_single;
	vector<vector<double> > path_diameters;
	vector<pair<int_type_t, int_type_t> > distance_node_map;
	vector<int_type_t> further_starts;
	int_type_t index_first_path;
	vector<SkelGraph> MasterSkeletonGraph;
	vector<vector<SkelGraph> > SkeletonGraphCC;

	vector< vector<vector<int_type_t> > > segment_paths_by_cc;
	vector<int_type_t> loop_counter_by_cc;
	vector<int_type_t> size_paths_by_loop;

	int_type_t number_nodes;
	vector<int_type_t> local_maxes_skeleton_hypotheses;

	in.open(supplied_BBfile.c_str());

	in >> division;
	in >> pA[0] >> pA[1] >> pA[2];
	in >> pB[0] >> pB[1] >> pB[2];

	object_file = supplied_object_file;
	write_directory = supplied_write_directory;

	BB.push_back(pA);
	BB.push_back(pB);

	// this version only does L2 induction
	switch (dist_type){
	case L2_induction: {} break;
	default: {
		cout << "Bad cost function selection. " << endl;
		exit(1);
	}
	}

	string current_write_file = write_directory + "trace.txt";

	ReconstructionStructure R_model;
	int max_connectivity = 26;

	/////////////////////////////// Read Model //////////////////////////////////////////////////
	ifstream oin;
	oin.open(object_file.c_str());

	std::ofstream out;
	out.open(current_write_file.c_str());

	if (oin.fail()){
		oin.close();

		out << "file " << object_file << " returns error " << endl;
		exit(1);

	}	else {

		R_model.CreateGridMinimal(BB, division);

		string val;
		oin >> val;

		int_type_t index_i;
		while (val != "-1"){

			index_i = FromString<int_type_t>(val);
			R_model.configuration_grid[index_i] = false;

			oin >> val;
		}

		out << "Read from " << object_file << endl;

	}

	//////////////////////////////////////////////////////////  AFTER READING //////////////////////////
	// timer starts after loaded. ...
	auto t0 = std::chrono::high_resolution_clock::now();

	vector<bool> skeleton(R_model.number_voxels_grid, true);


	// 1.  create nodes from the grid -- with connections.
	//biggest_cc_total_cc_pair = CreateGraphFromGridReturnPair(R_model, SkeletonGraph, connected_components_info, dist_type);
	biggest_cc_total_cc_pair = CreateGraphFromGridReturnPair(R_model, MasterSkeletonGraph, connected_components_info, dist_type);
	// We select the largest connected component
	biggest_cc = biggest_cc_total_cc_pair.first;

	out << "File has " << biggest_cc_total_cc_pair.second << " connected components and user has selected to curve skeletonize ";
	if (read_all_connected_components){
		out << " all of them" << endl;
	}	else {
		out << "only the largest. " << endl;
	}

	for (int i = 0, in  = connected_components_info.size(); i < in; i++){
		out << "cc " << i << " size " << connected_components_info[i] << endl;
	}


	int cc_lower_limit = 1;
	int cc_upper_limit = connected_components_info.size() + 1;

	// cc's have an empty at 0, but we're ignoring that for now.
	loop_counter_by_cc.resize(cc_upper_limit, 0);
	for (int cc_counter = 0; cc_counter < cc_upper_limit; cc_counter++){

		SkeletonGraphCC.push_back(vector<SkelGraph>());

		segment_paths_by_cc.push_back(vector<vector<int_type_t> >());
	}

	if (!read_all_connected_components){
		cc_lower_limit = biggest_cc;
		cc_upper_limit = cc_lower_limit + 1;
		cout << "Alert!  This file had more than one component; we're only dealing with the largest one in this implementation." << endl;

	}



	for (int cc = cc_lower_limit; cc < cc_upper_limit; cc++){

		loop_counter = 0;
		size_paths_by_loop.clear();
		size_paths_by_loop.push_back(0);


		/// not the greatest on memory, going to be at peace with this at the moment.
		ReworkUsingOnlyBiggestConnectedComponent(R_model, MasterSkeletonGraph, SkeletonGraphCC[cc], cc);

		number_nodes = SkeletonGraphCC[cc].size();

		if (number_nodes == 1){
			segment_paths_by_cc[cc].push_back(vector<int_type_t>(0));

		}	else {

			// allocations of storage locations ... ///////////////////////////////

			bfs_label_outside_to_inside_double = new double[number_nodes];
			bfs_label_outside_to_inside_inverse_double = new double[number_nodes];
			bfs_label_node_to_node_double = new double[number_nodes];
			bfs_from_tip_double = new double[number_nodes];


			frontier = new bool[number_nodes];
			already_explored = new bool[number_nodes];
			this_voxel_already_treated = new bool[number_nodes];
			pre_skeleton_bool = new bool[number_nodes];
			stop_lines = new bool[number_nodes];
			stop_lines_copy = new bool[number_nodes];

			// clear everything -- most is initialized
			paths.clear();


			// Initialization
			for (int_type_t i = 0; i < number_nodes; i++){
				already_explored[i] =false;
				this_voxel_already_treated[i] = false;
			}
			for (int_type_t i = 0; i < number_nodes; i++){
				bfs_label_outside_to_inside_double[i] = 0;
				bfs_from_tip_double[i] = 0;
				frontier[i] = false;
			}

			big_big_number = R_model.number_voxels_grid*2;
			//////////////////////////////// End reading/initialization /////////////////////////////////

			// Step 1
			auto t_timer_distance0 = std::chrono::high_resolution_clock::now();
			// Step 1.1, compute d_i
			greatest_OtI_distance_double = BFS_general_outside_to_inside_master_induction1(SkeletonGraphCC[cc], bfs_label_outside_to_inside_double, R_model.number_voxels_per_dim,
					big_big_number,
					max_connectivity, frontier, dist_type, true);
			auto t_timer_distance1 = std::chrono::high_resolution_clock::now();
			cout << "Time for distance computation without a grid ... " << std::chrono::duration_cast<std::chrono::milliseconds>(t_timer_distance1 - t_timer_distance0).count()	<< " milliseconds "<< endl;

			// Step 1.2, find v^*
			seed_node_index = FindALocalMaximaForSeed(SkeletonGraphCC[cc], bfs_label_outside_to_inside_double, big_big_number);
			best_value_double = bfs_label_outside_to_inside_double[seed_node_index];
			local_maxima_sparse_rep_double.clear();
			local_maxima_sparse_rep_double.push_back(seed_node_index);

			// create inverse of the bfs_map to get w_i's
			for (int_type_t i = 0; i < number_nodes; i++){
				bfs_label_outside_to_inside_inverse_double[i] = greatest_OtI_distance_double - bfs_label_outside_to_inside_double[i];
			}


			// Step 2.1 -- compute BFS1 map.
			temp_vector.clear();
			temp_vector.resize(1, seed_node_index);
			greatest_mod_distance_double = BFS_search_from_start_set(SkeletonGraphCC[cc],
					bfs_label_outside_to_inside_inverse_double, bfs_label_node_to_node_double,
					big_big_number, frontier, temp_vector, dist_type, true);

			cout << "greatest distance " << greatest_mod_distance_double << "  seed node " << seed_node_index << endl;


			// setting initial conditions for the loop
			current_path_id = 1;
			SkeletonGraphCC[cc][seed_node_index].path_id = current_path_id;

			// Step 2.2 -- find local maxima in the labels.
			local_maxes_skeleton_hypotheses.clear();
			bool valid = FindLocalMaximaSurfaceSingle(SkeletonGraphCC[cc], bfs_label_node_to_node_double, already_explored,
					local_maxes_skeleton_hypotheses,R_model.number_voxels_per_dim,
					big_big_number, max_connectivity, this_voxel_already_treated);


			while (local_maxes_skeleton_hypotheses.size() > 0 && valid == true){
				loop_counter++;


				multiple_paths_from_same_start.clear();
				local_tip = local_maxes_skeleton_hypotheses[0];

				//cout << "Local tip " << local_tip << "  already treated " << this_voxel_already_treated[local_tip] << endl;

				if (this_voxel_already_treated[local_tip] == false){ // determine if this voxel has been tested already.

					pre_skeleton_nodes.clear();

					local_max_distance_double = BFS_search_from_tip_to_existing_skeleton(SkeletonGraphCC[cc], bfs_label_outside_to_inside_inverse_double,
							bfs_from_tip_double, big_big_number, local_tip, pre_skeleton_bool, stop_lines, frontier, dist_type);


					// make a copy ....this gets reset in a later step.
					for (int_type_t k = 0; k < number_nodes; k++){
						stop_lines_copy[k] = stop_lines[k];
					}

					pre_skel_distance_double.clear();


					keep_components.clear();
					IdentifyLocalMinimaFromPreSkeletonsSetInKeepUpdatedComponents(SkeletonGraphCC[cc], pre_skeleton_bool, stop_lines, pre_skeleton_nodes, bfs_from_tip_double, keep_components);


					for (int_type_t k = 0; k < pre_skeleton_nodes.size(); k++){
						pre_skel_distance_double.push_back(pair<int_type_t, double>(pre_skeleton_nodes[k], bfs_from_tip_double[pre_skeleton_nodes[k]]));
					}

					// sort by arrival ....

					std::sort(pre_skel_distance_double.begin(), pre_skel_distance_double.end(), distance_comp_function_skel_distance_double);

					path_counter = 0;
					last_path_added = paths.size();

					pending_paths.clear();
					for (int_type_t k = 0; k < pre_skeleton_nodes.size(); k++){
						current_path.clear();
						linked_path.clear();

						WalkPreSkeletonToTipToFindJunctionsWithDeepIntoCornersModificationConnectivity(SkeletonGraphCC[cc], bfs_from_tip_double, bfs_label_outside_to_inside_double,
								pre_skel_distance_double[k].first, current_path);


						FindLinks(SkeletonGraphCC[cc], bfs_label_node_to_node_double, current_path[0], linked_path);

						linked_path[0] = linked_path[1];  // housekeeping, linked path is only two elements long, we only need the second one.
						linked_path.pop_back();
						linked_path.insert(linked_path.end(), current_path.begin(), current_path.end());

						pending_paths.push_back(linked_path);
					}

					// pre Loop handling -- step 4.2 -- deal with some noisy regions
					if (pre_skeleton_nodes.size() > 1){
						vector< vector<int_type_t> > temp_pending_paths;

						for (int_type_t k = 0; k < pre_skeleton_nodes.size(); k++){
							walks_through_another_stop_set0 = true;
							walks_through_another_stop_set1 = false;
							int_type_t out_of_first_stop_set_index = pending_paths[k].size();

							int_type_t limit = pending_paths[k].size();
							if (limit > 10){ limit = 10;}


							// first one is the skeleton.
							for (int_type_t i = 1, in  = pending_paths[k].size(); i < in && walks_through_another_stop_set0 == true; i++){
								if (stop_lines_copy[pending_paths[k][i]] == false){
									walks_through_another_stop_set0 = false;
									out_of_first_stop_set_index = i;
								}
							}

							for (int_type_t i = out_of_first_stop_set_index, in  = pending_paths[k].size(); i < in && walks_through_another_stop_set1 == false; i++){
								if (stop_lines_copy[pending_paths[k][i]] == true){
									walks_through_another_stop_set1 = true;
								}
							}

							if (walks_through_another_stop_set1 == false){
								temp_pending_paths.push_back(pending_paths[k]);
							}	else {
								ClearConnectedComponent(SkeletonGraphCC[cc], pending_paths[k][out_of_first_stop_set_index - 1],  stop_lines_copy);
							}
						}

						temp_pending_paths.swap(pending_paths);

						// need to go through and zero out the extra stop set components as well.
					}

					//				cout << "prending paths size " << pending_paths.size() << endl;
					//				if (pending_paths.size() == 1){
					//					for (int pp = 0; pp < pending_paths[0].size(); pp++){
					//						cout << pending_paths[0][pp] << " ";
					//					}
					//					cout << endl;
					//				}
					if (pending_paths.size() == 0){
						//cout << "We have no paths" << endl;
						//exit(1);
					}	else {


						// the size of pending paths may have changed, here we have the true loops.
						// Step 4.2 -- loop handling
						if (pending_paths.size() > 1){ ///loop case
							int_type_t first_intersection = 0;
							//int_type_t first_path_max_index_of_intersection = 0;

							int_type_t first_path_id = current_path_id;
							int_type_t last_path_id = 0;

							// this changes the labels to be the maximum numbered-branch that is coicident with the current branch.
							// the last branch has all the same label.
							for (int_type_t k = 0; k < pending_paths.size(); k++){
								// don't overwrite the ends ....
								first_intersection = 0;
								for (int_type_t j = 1, jn = pending_paths[k].size(); j < jn && first_intersection == 0; j++){
									SkeletonGraphCC[cc][pending_paths[k][j]].path_id = current_path_id;
								}

								if (SkeletonGraphCC[cc][pending_paths[k][0]].path_id == 0){
									SkeletonGraphCC[cc][pending_paths[k][0]].path_id = current_path_id;
								}

								path_counter++;
								current_path_id++;
							}

							last_path_id = current_path_id - 1;
							bool stop_descent = false;
							int_type_t path_id_for_this_one;

							for (int_type_t k = 0; k < pending_paths.size() - 1; k++){
								path_id_for_this_one = k + first_path_id;

								stop_descent = false;
								for (int_type_t i = pending_paths[k].size(); i > 2 && stop_descent == false; i--){
									if (SkeletonGraphCC[cc][pending_paths[k][i-2]].path_id == path_id_for_this_one){
										stop_descent = true;
									}	else {
										pending_paths[k].pop_back();
									}
								}
								paths.push_back(pending_paths[k]);
							}

							path_id_for_this_one = pending_paths.size() - 1 + first_path_id;
							for (int_type_t k = 0; k < pending_paths.size() - 1; k++){
								if (SkeletonGraphCC[cc][pending_paths[k].back()].path_id == path_id_for_this_one){
									SkeletonGraphCC[cc][pending_paths[k].back()].path_id = 0;
								}
							}

							// pop off the 'tail' for this local max
							stop_descent = false;
							for (int_type_t i = pending_paths.back().size() + 1; i > 1 && stop_descent == false; i--){
								if (SkeletonGraphCC[cc][pending_paths[pending_paths.size() - 1][i-2]].path_id == 0){
									stop_descent = true;
								}	else {
									SkeletonGraphCC[cc][pending_paths[pending_paths.size() - 1].back()].path_id = 0;
									pending_paths[pending_paths.size() - 1].pop_back();
								}
							}

							// then rewrite
							for (int_type_t k = 0; k < pending_paths.size() - 1; k++){
								if (SkeletonGraphCC[cc][pending_paths[k].back()].path_id == 0){
									SkeletonGraphCC[cc][pending_paths[k].back()].path_id = path_id_for_this_one;
								}
							}

							//cout << "Line 910 " << endl;
							for (int_type_t k = 0; k < pending_paths.size(); k++){
								paths.push_back(pending_paths[k]);
							}
						}	else {
							// mark the tip
							this_voxel_already_treated[local_tip] = true;
							// non loop case

							if (current_path_id > 1){


								passed_spurious_tests = ClassifyUsingChiSquaredAssumption(SkeletonGraphCC[cc], pending_paths[0][0], stop_lines_copy, paths, R_model.number_voxels_per_dim,
										big_big_number, max_connectivity, pending_paths[0], threshold);

							}	else {
								passed_spurious_tests = true;
							}

							if (passed_spurious_tests){
								//	cout << "Entering 1394" << pending_paths.size() << endl;
								// this takes care of the case with more than one segment per tip

								for (int_type_t j = 1, jn = pending_paths[0].size(); j < jn; j++){
									SkeletonGraphCC[cc][pending_paths[0][j]].path_id = current_path_id;
								}
								if (SkeletonGraphCC[cc][pending_paths[0][0]].path_id == 0){
									SkeletonGraphCC[cc][pending_paths[0][0]].path_id = current_path_id;
								}

								path_counter++;

								current_path_id++;
								paths.push_back(pending_paths[0]);
							}	else {

								FillInTreatedMap(SkeletonGraphCC[cc], bfs_from_tip_double, this_voxel_already_treated, big_big_number);
							}
						}
						current_path.clear();

						// Step 2.1 -- update BFS1
						if (paths.size() != last_path_added){
							vector<int_type_t> all_paths = paths[last_path_added];

							for (int_type_t new_path = last_path_added + 1; new_path < paths.size(); new_path++){
								all_paths.insert(all_paths.end(), paths[new_path].begin(), paths[new_path].end());
							}
							greatest_mod_distance_double = BFS_search_from_start_set(SkeletonGraphCC[cc],
									bfs_label_outside_to_inside_inverse_double, bfs_label_node_to_node_double,
									big_big_number, frontier, all_paths, dist_type, false);
						}
					}

					// cleaup relevant for very thin regions.
					size_paths_by_loop.push_back(paths.size());
					for (int pc = size_paths_by_loop[loop_counter - 1], pc_n = size_paths_by_loop[loop_counter]; pc < pc_n; pc++){
						for (int_type_t j = 0, jn = paths[pc].size(); j < jn; j++){
							this_voxel_already_treated[paths[pc][j]] = true;
						}
					}

				}


				// Step 2.2 -- locate potential endpoints in BFS1 map
				// find a new local maxima for the next round .....
				local_maxes_skeleton_hypotheses.clear();

				//cout << "Loop counter " << loop_counter << endl;

				// it may be that we terminate b/c we can't find any more local maxima.
				valid = FindLocalMaximaSurfaceSingle(SkeletonGraphCC[cc], bfs_label_node_to_node_double, already_explored,
						local_maxes_skeleton_hypotheses,R_model.number_voxels_per_dim,
						big_big_number, max_connectivity, this_voxel_already_treated);
			}
			out.close();


			////*******************  This section helps with organizing the curve skeleton output into a graph edges ********************************////
			//// *******************    Not included in the WACV paper, basic housekeeping  ****************************************************************************/////



			BreakIntoSegments(R_model, SkeletonGraphCC[cc], paths, segment_paths_by_cc[cc]);

			delete [] bfs_label_outside_to_inside_double;
			delete [] bfs_label_outside_to_inside_inverse_double;
			delete [] bfs_label_node_to_node_double;
			delete [] bfs_from_tip_double;

			delete [] frontier;
			delete [] already_explored;
			delete [] this_voxel_already_treated;
			delete [] pre_skeleton_bool;
			delete [] stop_lines;
			delete [] stop_lines_copy;

			loop_counter_by_cc[cc] = loop_counter;

		}
	}

	// stop timer, start writing.
	auto t1 = std::chrono::high_resolution_clock::now();
	cout << "Time for skeletonization ... " << std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count()<< " milliseconds "<< endl;

	filename = write_directory + "details.txt";
	cout << "Filename " << filename << endl;
	out.open(filename.c_str());
	out << "time " << std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count()/1000.0 << " seconds "<< endl;
	for (int cc = cc_lower_limit; cc < cc_upper_limit; cc++){
		out << "Number of occupied voxels per connected component: " << SkeletonGraphCC[cc].size() << endl;
	}
	out << "Number possible voxels " << R_model.number_voxels_grid << endl;
	for (int cc = cc_lower_limit; cc < cc_upper_limit; cc++){
		out << "Number of proposed tips by connected component " << loop_counter_by_cc[cc] << endl;
	}

	out << "Threshold for spurious segment classification " << threshold << endl;
	out.close();

	string seg_dir = write_directory + "segments_by_cc/";
	string command = "mkdir " + seg_dir;
	int ret = system(command.c_str());

	ConversionClass* CC = 0;

	//record_xyz_rep = true;
	if (record_xyz_rep == true){
		CC = new ConversionClass(BB, division);
	}

	for (int cc = cc_lower_limit; cc < cc_upper_limit; cc++){
		vector<vector<double> > segment_path_diameters_null;

		for (int_type_t i = 0, in = segment_paths_by_cc[cc].size(); i < in; i++){
			vector<double> temp(segment_paths_by_cc[cc][i].size(), R_model.inter_voxel_distance);
			segment_path_diameters_null.push_back(temp);
		}

		filename = seg_dir + "cc_"+ ToString<int>(cc)+ "_segments_TabbMedeiros.ply";
		WritePathsRefined(R_model, SkeletonGraphCC[cc], segment_paths_by_cc[cc], segment_path_diameters_null, filename );


		int_type_t number_segments_graph = segment_paths_by_cc[cc].size();
		int_type_t number_elements;
		string write_file = seg_dir + "result_segments" + ToString<int>(cc) + ".txt";
		out.open(write_file.c_str());

		out << number_segments_graph << endl;
		for (int_type_t i = 0; i < number_segments_graph; i++){
			number_elements = segment_paths_by_cc[cc][i].size();
			out << number_elements << endl;

			for (int_type_t j = 0; j < number_elements; j++){
				out << SkeletonGraphCC[cc][segment_paths_by_cc[cc][i][j]].grid_id << " ";
			}

			out << endl;
		}

		out.close();

		if (record_xyz_rep){
			double xd, yd, zd;
			write_file = seg_dir + "result_segments_xyz" + ToString<int>(cc) + ".txt";
			out.open(write_file.c_str());

			out << number_segments_graph << endl;
			for (int_type_t i = 0; i < number_segments_graph; i++){
				number_elements = segment_paths_by_cc[cc][i].size();
				out << number_elements << endl;

				for (int_type_t j = 0; j < number_elements; j++){
					CC->ReturnXYZIndicesFromIndex(SkeletonGraphCC[cc][segment_paths_by_cc[cc][i][j]].grid_id, xd, yd, zd);

					out << xd << " " << yd << " " << zd << endl;
				}

				out << endl;
			}


			out.close();
		}


	}

	string skel_dir = write_directory + "skeletons_by_cc/";
	command = "mkdir " + skel_dir;

	ret = system(command.c_str());

	for (int cc = cc_lower_limit; cc < cc_upper_limit; cc++){

		for (int_type_t k = 0; k < skeleton.size(); k++){
			R_model.configuration_grid[k] = true;
		}

		for (int i = 0, in = SkeletonGraphCC[cc].size(); i < in; i++){
			R_model.configuration_grid[SkeletonGraphCC[cc][i].grid_id] = false;
		}

		for (int_type_t k = 0; k < skeleton.size(); k++){
			skeleton[k] = true;
		}

		for (int_type_t i = 0, in = segment_paths_by_cc[cc].size(); i < in; i++){
			for (int_type_t j = 0, jn= segment_paths_by_cc[cc][i].size(); j < jn; j++){
				skeleton[SkeletonGraphCC[cc][segment_paths_by_cc[cc][i][j]].grid_id] = false;
			}
		}


		string write_file = skel_dir + "result_" + ToString<int>(cc) + ".txt";
		out.open(write_file.c_str());

		uint64_t number_in_result = 0;
		for (int_type_t i = 0; i < R_model.number_voxels_grid; i++){
			if (skeleton[i] == false){
				out << i << " ";
				number_in_result++;
			}

		}
		out << "-1" << endl;

		out.close();

		if (record_xyz_rep == true)
		{
			double xd, yd, zd;

			write_file = skel_dir + "result_xyz" + ToString<int>(cc) + ".txt";
			out.open(write_file.c_str());
			out << number_in_result << endl;
			for (int_type_t i = 0; i < R_model.number_voxels_grid; i++){
				if (skeleton[i] == false){
					CC->ReturnXYZIndicesFromIndex(i, xd, yd, zd);
					out << xd << " " << yd << " " << zd << endl;
				}
			}

			out.close();


		}




		R_model.GenerateAndWriteSurfaceInPlyFormat(skel_dir, cc, "initial", NULL, false);
		R_model.configuration_grid = skeleton;

		R_model.GenerateAndWriteSurfaceInPlyFormat(skel_dir, cc, "skel_TabbMedeiros_", NULL, false);

	}


	if (record_xyz_rep == true){
		// combine all skeletons for all connected components together -- relevant for image stacks.

		for (int_type_t k = 0; k < skeleton.size(); k++){
			skeleton[k] = true;
		}

		for (int cc = cc_lower_limit; cc < cc_upper_limit; cc++){
			for (int_type_t i = 0, in = segment_paths_by_cc[cc].size(); i < in; i++){
				for (int_type_t j = 0, jn= segment_paths_by_cc[cc][i].size(); j < jn; j++){
					skeleton[SkeletonGraphCC[cc][segment_paths_by_cc[cc][i][j]].grid_id] = false;
				}
			}
		}


		string write_file = write_directory + "result.txt";
		out.open(write_file.c_str());

		uint64_t number_in_result = 0;
		for (int_type_t i = 0; i < R_model.number_voxels_grid; i++){
			if (skeleton[i] == false){
				out << i << " ";
				number_in_result++;
			}

		}
		out << "-1" << endl;

		out.close();


		delete CC;
	}

}



