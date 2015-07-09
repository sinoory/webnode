#ifndef UILOG_H
#define UILOG_H
#include <QString>
class UIlog
{
public:
    UIlog();
    void uilog_init();
    void uilog_write(QString log_info);
    void uilog_quit();

private:
    bool is_init;
};

#endif // UILOG_H
