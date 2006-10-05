require 'icpc'
require 'structures'
require 'journal'
	
class ICPC
	include MathUtils	
	include Math
	include Icpc
	
	attr_accessor :params
	
	def initialize
		@params = standard_parameters
	end
	
	def name 
		"ICPC"
	end

	def scan_matching
		
		# pass all parameters to extension library
		laser_ref  = params[:laser_ref]
		laser_sens = params[:laser_sens]
		
		icpc_l_nrays(0, laser_ref.nrays);
		icpc_l_min_theta(0, laser_ref.min_theta);
		icpc_l_max_theta(0, laser_ref.max_theta);
		laser_ref.points.each_index do |j|
			icpc_l_ray(0, j, laser_ref.points[j].theta, laser_ref.points[j].reading)
		end

		icpc_l_nrays(1, laser_sens.nrays);
		icpc_l_min_theta(1, laser_sens.min_theta);
		icpc_l_max_theta(1, laser_sens.max_theta);
		laser_sens.points.each_index do |i|
			icpc_l_ray(1, i, laser_sens.points[i].theta, laser_sens.points[i].reading)
		end
	
		# pass parameters 
		Icpc::icpc_params.maxAngularCorrectionDeg = params[:maxAngularCorrectionDeg]
		Icpc::icpc_params.maxLinearCorrection     = params[:maxLinearCorrection]
		Icpc::icpc_params.maxIterations           = params[:maxIterations]
		Icpc::icpc_params.epsilon_xy              = params[:epsilon_xy]
		Icpc::icpc_params.epsilon_theta           = params[:epsilon_theta]
		Icpc::icpc_params.maxCorrespondenceDist   = params[:maxCorrespondenceDist]
		
		u=params[:firstGuess];
		icpc_odometry(u[0],u[1],u[2]);

		icpc_go
		
		res = Hash.new 
#		puts "Class is #{Icpc::icpc_res.x.methods.join(', ')}"
#		puts "Class is #{Icpc::icpc_res.x.to_a}"
		x = Icpc::icpc_get_x();
		res[:x] = Vector[x[0],x[1],x[2]]
		res[:iterations] = Icpc::icpc_res.iterations;
		
		icpc_cleanup
		
		res
	end
	
	
	def journal_open(filename)
		icpc_init_journal(filename)
	end
end