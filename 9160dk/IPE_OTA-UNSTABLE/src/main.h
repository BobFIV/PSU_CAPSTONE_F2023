#ifndef MAIN_H
#define MAIN_H
//why the heck do I need to do this...
extern struct mqtt_client client;
extern struct k_work periodic_publish_work;
extern struct k_work periodic_transmit_work;




#endif // MAIN_H
