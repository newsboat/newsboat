#include <iostream>
using namespace std;

class Number
{
private:
    int x, y;
public:
    Number(){}
    Number (int x, int y)
    {
        this->x = x;
        this->y = y;
        cout << "Your number     X: " << x << "\n\t\tY:  " << y <<endl;
    }

    bool operator ==(const Number &mynum)
    {
        return (this->x == mynum.x && this->y == mynum.y);
    }

    bool operator >(const Number &mynum)
    {
        return (this->x > mynum.x && this->y > mynum.y);
    }


    
    Number & operator++ (){
        this->x++;
        this->y++;
        cout << x << " " << y << endl;
        return *this;
    }

    Number operator+(const Number &mynum)
    {
        Number temp;
        temp.x = this->x + mynum.x;
        temp.y = this->y + mynum.y;
        cout <<  temp.x << "  " << temp.y << endl;
        return temp;
    }

};
int main()
{
    Number num1(2, 65);
    Number num2(35, 1);
    bool res = num1 == num2;
    cout << "\t" << res << endl;
    bool res2 = num1 > num2;
    cout << "\t" << res2 << endl;
    Number sum;
    sum = num1 + num2;
    ++num1;
    ++num2;
    return 0;
}
