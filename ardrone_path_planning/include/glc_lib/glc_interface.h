#ifndef USER_INTERFACE_H
#define USER_INTERFACE_H

//Includes
#include "math_utils.h"
#include "glc_utils.h"

namespace glc{
  
  class Inputs{
  public:
    std::deque<vctr> points;//TODO private and
    
    void addInputSample(vctr& _input)
    {
      points.push_back(_input);
    }
  };
  
  class Heuristic{
  protected:
    vctr goal;
  public: 
    virtual double costToGo(const vctr& x0)=0;
    void setGoal(vctr _goal){goal=_goal;}
  }; 
  
  class CostFunction{
  protected:
    const double lipschitz_constant;
  public:
    CostFunction(double _lipschitz_constant):lipschitz_constant(_lipschitz_constant){}
    
    virtual double cost(const Trajectory& candidate, const vctr& u)=0;
    
    double getLipschitzConstant(){
      return lipschitz_constant;
    }
  };
  
  class GoalRegion{
  public:
    bool inGoal(const Trajectory& x, int* last=NULL){ 
      for(int i=0;i<x.size();i++) {
        if(inGoal(x.getState(i), x.getTime(i))){
          if(last)
            *last = i;
          return true;
        }
      }
      return false;
    }
    virtual bool inGoal(const vctr& state, const double& time)=0;
    
  };
  
  class Obstacles{
  public:
    int collision_counter=0;
    //check pointwise for collision    
    virtual bool collisionFree(const Trajectory& x, int* last=NULL){ 
      for(int i=0;i<x.size();i++) {
        if(not collisionFree(x.getState(i), x.getTime(i))){
          if(last)
            *last = i;
          return false;
        }
      }
      return true;
    }
    
    virtual bool collisionFree(const glc::vctr& x, const double& t)=0;
  };
  
  //Vector field with numerical integration
  class DynamicalSystem{
  public:
    const double max_time_step;
    int sim_counter=0;
    
    DynamicalSystem(double _max_time_step):max_time_step(_max_time_step){}
    
    virtual void flow(vctr& dx, const vctr& x,const vctr& u)=0;
    
    virtual double getLipschitzConstant()=0;
    
    //Forward integration step
    virtual void step(vctr& x_plus, const vctr& x, const vctr& u, const double dt)=0;
    
    void sim(glc::Trajectory& sol, double t0, double tf, const vctr& x0, const vctr& u){
      assert(tf>t0);
      
      //compute minimum number of steps to satisfy dt<dt_max
      double num_steps=ceil((tf-t0)/max_time_step);
      double dt=(tf-t0)/num_steps;
      //resize solution
      sol.reserve(num_steps+1);
      //set initial condition
      sol.push_back(x0,t0);
      vctr x1;
      //integrate
      for(int i=0;i<num_steps;i++){
        step(x1,sol.getState(i),u,dt);
        sol.push_back(x1, sol.getTime(i)+dt);
      }
      sim_counter++;
      return;
    }
    
  };
  
  class EulerIntegrator : public DynamicalSystem{
    vctr xdot;//For multithreading move into step function
  public:
    EulerIntegrator(int _dimension, double _max_time_step):DynamicalSystem(_max_time_step),xdot(_dimension){}
    void step(vctr& x_plus, const vctr& x, const vctr& u, const double dt) override {  
      flow(xdot,x,u);
      x_plus = x+dt*xdot;
    }
  };
  
  class RungeKutta4 : public DynamicalSystem{
    vctr k1,k2,k3,k4,temp;//For multithreading move into step function
  public:
    RungeKutta4(double _max_time_step):DynamicalSystem(_max_time_step){}
    
    void step(vctr& x_plus, const vctr& x, const vctr& u, const double dt) override {  
      static double SIXTH = 0.166666666666666;
      
      flow(k1,x,u);
      temp=x+0.5*dt*k1;
      flow(k2,temp,u);
      temp=x+0.5*dt*k2;
      flow(k3,temp,u);
      temp=x+dt*k3;
      flow(k4,temp,u);
      temp=k1+2.0*k2+2.0*k3+k4;
      
      x_plus = x+dt*SIXTH*temp;
    }
  };
  
  
  class SingleIntegrator : public glc::EulerIntegrator {
  public:
    SingleIntegrator(int _dimension, const double& _max_time_step): EulerIntegrator(_dimension,_max_time_step) {}
    
    void flow(vctr& dx, const vctr& x, const vctr& u) override {
      dx=u;
    }
    
    double getLipschitzConstant() override {
      return 0.0;
    }
  };
}

#endif