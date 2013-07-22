/*
 * Tickable.h
 *
 * Definition of tickable super-class.
 * Any class which should be tickled by the TickHandler
 * must extend this class.
 *
 *  Created on: 21.07.2013
 *      Author: Michael Neuweiler
 */

#ifndef TICKABLE_H_
#define TICKABLE_H_


class Tickable {
public:
	virtual void setup();
	virtual void handleTick();

protected:

private:

};


#endif /* TICKABLE_H_ */
