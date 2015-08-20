#ifndef PACKET_H
#define PACKET_H

typedef struct __attribute__((__packed__)) {
	uint8_t id_from;		// the id of the critter who sent at a certain transmit power
	uint8_t id_to;
	uint16_t local_time;	// timestamp this meetup occurred: gathered form RTC of sender
} sync_t;

sync_t create_sync_packet(uint8_t id_sender, uint8_t rcv, uint16_t local_time) {
	sync_t pkt;
	pkt.id_from = id_sender;
	pkt.id_to = rcv;
	pkt.local_time = local_time;
	return pkt;
}

sync_t decode_sync_packet(uint8_t *buf) {
	sync_t tmp;	//Re-make the struct
	memcpy(&tmp, buf, sizeof(tmp));
	return tmp;
}

#endif