#include <stdio.h>
#include "ibm.h"
#include "io.h"
#include "nvr.h"
#include "pic.h"
#include "timer.h"

int oldromset;
int nvrmask=63;
uint8_t nvrram[128];
int nvraddr;

int nvr_dosave = 0;

static int nvr_onesec_time = 0, nvr_onesec_cnt = 0;

int enable_sync = 0;

int second, minute, hour, day, month, year;
int days_in_month[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

static int is_leap(int year)
{
	if (year % 400 == 0)  return 1;
	if (year % 100 == 0)  return 0;
	if (year % 4 == 0)  return 1;
	return 0;
}

static int get_days(int month, int year)
{
	if (month != 2)
	{
		return days_in_month[month];
	}
	else
	{
		return is_leap(year) ? 29 : 28;
	}
}

static int convert_to_bcd(int number)
{
	int n1, n2;
	n1 = number % 10;
	n2 = number - n1;
	n2 /= 10;
	n2 <<= 4;
	return (n2 | n1);
}

static int convert_from_bcd(int number)
{
	int n1, n2;
	n1 = number & 0xF;
	n2 = number >> 4;
	n2 *= 10;
	return (n2 + n1);
}

static int final_form(int isbcd, int number)
{
	return isbcd ? convert_to_bcd(number) : number;
}

static int original_form(int isbcd, int number)
{
	return isbcd ? convert_from_bcd(number) : number;
}

static void nvr_update_internal_clock()
{
	second++;
	if (second == 60)
	{
		second = 0;
		minute++;
	}
	if (minute == 60)
	{
		minute = 0;
		hour++;
	}
	if (hour == 24)
	{
		hour = 0;
		day++;
	}
	if (day == (get_days(month, year) + 1))
	{
		day = 1;
		month++;
	}
	if (month == 13)
	{
		month = 1;
		year++;
	}
}

static int to_12_hour(int hour)
{
	int hour2 = hour;
	hour2 %= 12;
	if (!hour2)  hour2 = 12;
	return hour2;
}

static int from_12_hour(int hour)
{
	int hour2 = hour & 0x7F;
	if (hour2 == 12)  hour2 == 0;
	if (hour & 0x80)  hour2 += 12;
	return hour2;
}

static int week_day()
{
	int day_of_month = day;
	int month2 = month;
	int year2 = year % 100;
	int century = ((year - year2) / 100) % 4;
	int sum = day_of_month + month2 + year2 + century;
	/* (sum mod 7) gives 0 for Saturday, we need it for Monday, so +5 */
	int raw_wd = ((sum + 5) % 7);
	/* +1 so 1 = Monday, 7 = Sunday */
	return raw_wd + 1;
}

static void update_reg_0B(int val)
{
	int old_24 = (nvrram[0xB] & 2) ? 1 : 0;
	int new_24 = (val & 2) ? 1 : 0;
	int old_bcd = (nvrram[0xB] & 4) ? 0 : 1;
	int new_bcd = (val & 4) ? 0 : 1;

	uint8_t baknvr[10];

	if (AMSTRAD)  return;

	memcpy(baknvr,nvrram,10);

	/* First convert to regular form. */
	second = original_form(old_bcd, nvrram[0]);
	minute = original_form(old_bcd, nvrram[2]);
	hour = old_24 ? original_form(old_bcd, nvrram[4]) : from_12_hour(original_form(old_bcd, nvrram[4]));
	day = original_form(old_bcd, nvrram[7]);
	month = original_form(old_bcd, nvrram[8]);
	year = original_form(old_bcd, nvrram[9]) + 1900;

	/* Then convert to new form. */
	nvrram[0] = final_form(new_bcd, second);
	nvrram[2] = final_form(new_bcd, minute);
	nvrram[4] = new_24 ? final_form(new_bcd, hour) : final_form(new_bcd, to_12_hour(hour));
	nvrram[6] = week_day();
	nvrram[7] = final_form(new_bcd, day);
	nvrram[8] = final_form(new_bcd, month);
	nvrram[9] = final_form(new_bcd, (year % 100));

        if (baknvr[0] != nvrram[0] ||
            baknvr[2] != nvrram[2] ||
            baknvr[4] != nvrram[4] ||
            baknvr[6] != nvrram[6] ||
            baknvr[7] != nvrram[7] ||
            baknvr[8] != nvrram[8] ||
            baknvr[9] != nvrram[9])
		nvrram[0xA]|=0x80;
}

/* Called on every get time. */
static void set_registers()
{
	int is24hour = (nvrram[0xB] & 2) ? 1 : 0;
	int isbcd = (nvrram[0xB] & 4) ? 0 : 1;

	uint8_t baknvr[10];

	memcpy(baknvr,nvrram,10);

	if (AMSTRAD)
	{
		is24hour = 1;
		isbcd = 1;
	}

	nvrram[0] = final_form(isbcd, second);
	nvrram[2] = final_form(isbcd, minute);
	nvrram[4] = is24hour ? final_form(isbcd, hour) : final_form(isbcd, to_12_hour(hour));
	nvrram[6] = week_day();
	nvrram[7] = final_form(isbcd, day);
	nvrram[8] = final_form(isbcd, month);
	nvrram[9] = final_form(isbcd, (year % 100));

        if (baknvr[0] != nvrram[0] ||
            baknvr[2] != nvrram[2] ||
            baknvr[4] != nvrram[4] ||
            baknvr[6] != nvrram[6] ||
            baknvr[7] != nvrram[7] ||
            baknvr[8] != nvrram[8] ||
            baknvr[9] != nvrram[9])
		nvrram[0xA]|=0x80;
}

/* Called on NVR load and write. */
static void get_registers()
{
	int is24hour = (nvrram[0xB] & 2) ? 1 : 0;
	int isbcd = (nvrram[0xB] & 4) ? 0 : 1;

	int temp_hour = 0;

	if (AMSTRAD)
	{
		is24hour = 1;
		isbcd = 1;
	}

	second = original_form(isbcd, nvrram[0]);
	minute = original_form(isbcd, nvrram[2]);
	hour = is24hour ? original_form(isbcd, nvrram[4]) : from_12_hour(original_form(isbcd, nvrram[4]));
	day = original_form(isbcd, nvrram[7]);
	month = original_form(isbcd, nvrram[8]);
	year = original_form(isbcd, nvrram[9]) + 1900;
}

void getnvrtime()
{
	if (enable_sync)
	{
		/* Get time from host. */
		time_get(nvrram);
	}
	else
	{
		/* Get time from internal clock. */
		set_registers();
	}
}

void update_sync()
{
	if (enable_sync)
	{
		/* Get time from host. */
		time_get(nvrram);
	}
	else
	{
		/* Save time to registers but keep it as is. */
		get_registers();
	}
}

void nvr_recalc()
{
        int c;
        int newrtctime;
        c=1<<((nvrram[0xA]&0xF)-1);
        newrtctime=(int)(RTCCONST * c * (1 << TIMER_SHIFT));
        if (rtctime>newrtctime) rtctime=newrtctime;
}

void nvr_rtc(void *p)
{
        int c;
        if (!(nvrram[0xA]&0xF))
        {
                rtctime=0x7fffffff;
                return;
        }
        c=1<<((nvrram[0xA]&0xF)-1);
        rtctime += (int)(RTCCONST * c * (1 << TIMER_SHIFT));
//        pclog("RTCtime now %f\n",rtctime);
        nvrram[0xC] |= 0x40;
        if (nvrram[0xB]&0x40)
        {
                nvrram[0xC]|=0x80;
                if (AMSTRAD) picint(2);
                else         picint(0x100);
//                pclog("RTC int\n");
        }
}

void nvr_onesec(void *p)
{
        nvr_onesec_cnt++;
        if (nvr_onesec_cnt >= 100)
        {
                nvr_onesec_cnt = 0;
		/* If sync is disabled, move internal clock ahead by 1 second. */
		if (!enable_sync)  nvr_update_internal_clock();
                nvrram[0xC] |= 0x10;
                if (nvrram[0xB] & 0x10)
                {
                        nvrram[0xC] |= 0x80;
                        if (AMSTRAD) picint(2);
                        else         picint(0x100);
                }
//                pclog("RTC onesec\n");
        }
        nvr_onesec_time += (int)(10000 * TIMER_USEC);
}

void writenvr(uint16_t addr, uint8_t val, void *priv)
{
        int c, old;
//        printf("Write NVR %03X %02X %02X %04X:%04X %i\n",addr,nvraddr,val,cs>>4,pc,ins);
        if (addr&1)
        {
//                if (nvraddr == 0x33) pclog("NVRWRITE33 %02X %04X:%04X %i\n",val,CS,pc,ins);
		old = nvrram[nvraddr];
                if (nvraddr >= 0xe && nvrram[nvraddr] != val) 
                   nvr_dosave = 1;
		if (nvraddr==0xB)  update_reg_0B(val);
		if (nvraddr!=0xC && nvraddr!=0xD) nvrram[nvraddr]=val;

		/* If not syncing the time with the host, we need to update our internal clock on write. */
		if (!enable_sync)
		{
			switch(nvraddr)
			{
				case 0:
				case 2:
				case 4:
				case 6:
				case 7:
				case 8:
				case 9:
					if (old != val)
					{
						get_registers();
						nvr_dosave = 1;
					}
					return;
			}
		}

                if (nvraddr==0xA)
                {
//                        pclog("NVR rate %i\n",val&0xF);
                        if (val&0xF)
                        {
                                c=1<<((val&0xF)-1);
                                rtctime += (int)(RTCCONST * c * (1 << TIMER_SHIFT));
                        }
                        else
                           rtctime = 0x7fffffff;
                }
        }
        else        nvraddr=val&nvrmask;
}

uint8_t readnvr(uint16_t addr, void *priv)
{
        uint8_t temp;
//        printf("Read NVR %03X %02X %02X %04X:%04X\n",addr,nvraddr,nvrram[nvraddr],cs>>4,pc);
        if (addr&1)
        {
                if (nvraddr<=0xA) getnvrtime();
                if (nvraddr==0xD) nvrram[0xD]|=0x80;
                if (nvraddr==0xA)
                {
                        temp=nvrram[0xA];
                        nvrram[0xA]&=~0x80;
                        return temp;
                }
                if (nvraddr==0xC)
                {
                        if (AMSTRAD) picintc(2);
                        else         picintc(0x100);
                        temp=nvrram[0xC];
                        nvrram[0xC]=0;
                        return temp;
                }
//                if (AMIBIOS && nvraddr==0x36) return 0;
//                if (nvraddr==0xA) nvrram[0xA]^=0x80;
                return nvrram[nvraddr];
        }
        return nvraddr;
}

void loadnvr()
{
        FILE *f;
        int c;
        nvrmask=63;
        oldromset=romset;
        switch (romset)
        {
                case ROM_PC1512:      f = romfopen("pc1512.nvr",      "rb"); break;
                case ROM_PC1640:      f = romfopen("pc1640.nvr",      "rb"); break;
                case ROM_PC200:       f = romfopen("pc200.nvr",       "rb"); break;
                case ROM_PC2086:      f = romfopen("pc2086.nvr",      "rb"); break;
                case ROM_PC3086:      f = romfopen("pc3086.nvr",      "rb"); break;                
                case ROM_IBMAT:       f = romfopen("at.nvr",          "rb"); break;
                case ROM_IBMPS1_2011: f = romfopen("ibmps1_2011.nvr", "rb"); /*nvrmask = 127; */break;
                case ROM_CMDPC30:     f = romfopen("cmdpc30.nvr",     "rb"); nvrmask = 127; break;
                case ROM_AMI286:      f = romfopen("ami286.nvr",      "rb"); nvrmask = 127; break;
                case ROM_DELL200:     f = romfopen("dell200.nvr",     "rb"); nvrmask = 127; break;
                case ROM_IBMAT386:    f = romfopen("at386.nvr",       "rb"); nvrmask = 127; break;
                case ROM_DESKPRO_386: f = romfopen("deskpro386.nvr",  "rb"); break;
                case ROM_ACER386:     f = romfopen("acer386.nvr",     "rb"); nvrmask = 127; break;
                case ROM_MEGAPC:      f = romfopen("megapc.nvr",      "rb"); nvrmask = 127; break;
                case ROM_AMI386:      f = romfopen("ami386.nvr",      "rb"); nvrmask = 127; break;
                case ROM_AMI486:      f = romfopen("ami486.nvr",      "rb"); nvrmask = 127; break;
                case ROM_WIN486:      f = romfopen("win486.nvr",      "rb"); nvrmask = 127; break;
                case ROM_PCI486:      f = romfopen("hot-433.nvr",     "rb"); nvrmask = 127; break;
                case ROM_SIS496:      f = romfopen("sis496.nvr",      "rb"); nvrmask = 127; break;
                case ROM_430VX:       f = romfopen("430vx.nvr",       "rb"); nvrmask = 127; break;
                case ROM_REVENGE:     f = romfopen("revenge.nvr",     "rb"); nvrmask = 127; break;
                case ROM_ENDEAVOR:    f = romfopen("endeavor.nvr",    "rb"); nvrmask = 127; break;
                case ROM_PX386:       f = romfopen("px386.nvr",       "rb"); nvrmask = 127; break;
                case ROM_DTK386:      f = romfopen("dtk386.nvr",      "rb"); nvrmask = 127; break;
                case ROM_DTK486:      f = romfopen("dtk486.nvr",      "rb"); nvrmask = 127; break;
                case ROM_R418:        f = romfopen("r418.nvr",        "rb"); nvrmask = 127; break;
                case ROM_PLATO:       f = romfopen("plato.nvr",       "rb"); nvrmask = 127; break;
                case ROM_MB500N:      f = romfopen("mb500n.nvr",      "rb"); nvrmask = 127; break;
                case ROM_ACERM3A:     f = romfopen("acerm3a.nvr",     "rb"); nvrmask = 127; break;
                case ROM_ACERV35N:    f = romfopen("acerv35n.nvr",    "rb"); nvrmask = 127; break;
                case ROM_P55T2P4:     f = romfopen("p55t2p4.nvr",     "rb"); nvrmask = 127; break;
                case ROM_P55VA:       f = romfopen("p55va.nvr",       "rb"); nvrmask = 127; break;
                default: return;
        }
        if (!f)
        {
                memset(nvrram,0xFF,128);
                return;
        }
        fread(nvrram,128,1,f);
        fclose(f);
        nvrram[0xA]=6;
        nvrram[0xB]=0;
        c=1<<((6&0xF)-1);
        rtctime += (int)(RTCCONST * c * (1 << TIMER_SHIFT));
	/* If sync is disabled, load internal clock from registers. */
	if (!enable_sync)  get_registers();
}
void savenvr()
{
        FILE *f;
        switch (oldromset)
        {
                case ROM_PC1512:      f = romfopen("pc1512.nvr",      "wb"); break;
                case ROM_PC1640:      f = romfopen("pc1640.nvr",      "wb"); break;
                case ROM_PC200:       f = romfopen("pc200.nvr",       "wb"); break;
                case ROM_PC2086:      f = romfopen("pc2086.nvr",      "wb"); break;
                case ROM_PC3086:      f = romfopen("pc3086.nvr",      "wb"); break;
                case ROM_IBMAT:       f = romfopen("at.nvr",          "wb"); break;
                case ROM_IBMPS1_2011: f = romfopen("ibmps1_2011.nvr", "wb"); break;
                case ROM_CMDPC30:     f = romfopen("cmdpc30.nvr",     "wb"); break;                
                case ROM_AMI286:      f = romfopen("ami286.nvr",      "wb"); break;
                case ROM_DELL200:     f = romfopen("dell200.nvr",     "wb"); break;
                case ROM_IBMAT386:    f = romfopen("at386.nvr",       "wb"); break;
                case ROM_DESKPRO_386: f = romfopen("deskpro386.nvr",  "wb"); break;
                case ROM_ACER386:     f = romfopen("acer386.nvr",     "wb"); break;
                case ROM_MEGAPC:      f = romfopen("megapc.nvr",      "wb"); break;
                case ROM_AMI386:      f = romfopen("ami386.nvr",      "wb"); break;
                case ROM_AMI486:      f = romfopen("ami486.nvr",      "wb"); break;
                case ROM_WIN486:      f = romfopen("win486.nvr",      "wb"); break;
                case ROM_PCI486:      f = romfopen("hot-433.nvr",     "wb"); break;
                case ROM_SIS496:      f = romfopen("sis496.nvr",      "wb"); break;
                case ROM_430VX:       f = romfopen("430vx.nvr",       "wb"); break;
                case ROM_REVENGE:     f = romfopen("revenge.nvr",     "wb"); break;
                case ROM_ENDEAVOR:    f = romfopen("endeavor.nvr",    "wb"); break;
                case ROM_PX386:       f = romfopen("px386.nvr",       "wb"); break;
                case ROM_DTK386:      f = romfopen("dtk386.nvr",      "wb"); break;
                case ROM_DTK486:      f = romfopen("dtk486.nvr",      "wb"); break;
                case ROM_R418:        f = romfopen("r418.nvr",        "wb"); break;
                case ROM_PLATO:       f = romfopen("plato.nvr",       "wb"); break;
                case ROM_MB500N:      f = romfopen("mb500n.nvr",      "wb"); break;
                case ROM_ACERM3A:     f = romfopen("acerm3a.nvr",     "wb"); break;
                case ROM_ACERV35N:    f = romfopen("acerv35n.nvr",    "wb"); break;
                case ROM_P55T2P4:     f = romfopen("p55t2p4.nvr",     "wb"); break;
                case ROM_P55VA:       f = romfopen("p55va.nvr",       "wb"); break;
                default: return;
        }
	/* If sync is disabled, save internal clock to registers. */
	if (!enable_sync)  set_registers();
        fwrite(nvrram,128,1,f);
        fclose(f);
}

void nvr_init()
{
        io_sethandler(0x0070, 0x0002, readnvr, NULL, NULL, writenvr, NULL, NULL,  NULL);
        timer_add(nvr_rtc, &rtctime, TIMER_ALWAYS_ENABLED, NULL);
        timer_add(nvr_onesec, &nvr_onesec_time, TIMER_ALWAYS_ENABLED, NULL);
}
