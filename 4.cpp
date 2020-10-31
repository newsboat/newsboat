#include <iostream>  

using namespace std;

class Boots
{
public:
     void Boots_Animal()
     {
         cout << "The Best boots" << endl;
     }
};

Boots boots;


class Cat
{
private:
    class Heart
    {
    public:
        void Hearts()
        {
            cout << "I live" << endl;
        }
    };
Heart heart;
public:
    void Cat_live()
    {
        boots.Boots_Animal();
        heart.Hearts();
    }

};

int main(int argc, char const *argv[])
{
    Cat cat;
    cat.Cat_live();
    cout << "You can use this boots again " ;boots.Boots_Animal();
    return 0;
}
