#include <iostream>
#include <math.h>
#include <vector>

using namespace std;  

class Cout_vectror
{
public:
    virtual void i_vector(vector<int> g1) = 0;
}; 

class Cout_vectror1 : public Cout_vectror
{
    private:
    vector<int> g1;
public:
    void i_vector(vector<int> g1) override
    {
        this->g1 = g1; 
    cout << "Output of begin and end: "; 
     for (auto i = g1.begin(); i != g1.end(); ++i) 
         cout << *i << " ";  
    }
};
class Cout_vectror2 : public Cout_vectror
{
private:
    vector<int> g1;
public:
    void i_vector(vector<int> g1) override
    {
    this->g1 = g1; 
    cout << "\nOutput of rbegin and rend: "; 
    for (auto ir = g1.rbegin(); ir != g1.rend(); ++ir) 
        cout << *ir << " ";   
    }
};
class Cout_vectror3 : public Cout_vectror
{
private:
    vector<int> g1;
public:
    void i_vector(vector<int> g1) override
    {
    this->g1 = g1; 
    cout << endl;
    g1.erase(g1.begin());
    cout << "Output without first element "; 
     for (auto i = g1.begin(); i != g1.end(); ++i) 
         cout << *i << " ";  
    }
};

class All
{
public:
    void i_vector(Cout_vectror1 *vec, Cout_vectror2 *vec2, Cout_vectror3 *vec3)
    {
        vector<int> g1;
        for (int i = 1; i <= 5; i++) 
        {
            g1.push_back(i); 
        }
        vec->i_vector(g1);
        vec2->i_vector(g1); 
        vec3->i_vector(g1); 
    }
};

int main()
{

    Cout_vectror1 vec;
    Cout_vectror2 vec2;
    Cout_vectror3 vec3;
    All all;
    all.i_vector(&vec, &vec2, &vec3);
    return 0;
}
