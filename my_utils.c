
void staller(int dur) {
    long i;
    for (i = 0; i < dur * 1000000; i++)
            asm("nop");
}

void stall_ms(long dur) {
    long i;
    for (i = 0; i < dur; i++)
            asm("nop");
}


