/*
 *	freediag - Vehicle Diagnostic Utility
 *
 *
 * Copyright (C) 2001 Richard Almeida & Ibex Ltd (rpa@ibex.co.uk)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *************************************************************************
 *
 *
 * Mostly ODBII Compliant Scan Tool (as defined in SAE J1978)
 *
 * CLI routines - "set" commands
 *
 *
 */
#include <stdbool.h>

#include "diag.h"
#include "diag_l0.h"
#include "diag_l1.h"
#include "diag_l2.h"

#include "scantool.h"
#include "scantool_cli.h"

#define PROTO_NONE	"<not_used>"

/** Global parameters **/
/* struct global_cfg contains all global parameters */
struct globcfg global_cfg;

/** WIP : convert the following items **/
struct diag_l0_device *test_dl0d;	//global dl0d test

unsigned int set_speed;	/* Comms speed */

const char *	set_vehicle;	/* Vehicle */
const char *	set_ecu;	/* ECU name */

#define DEFAULT_INTERFACE CARSIM	//index into l0_names below
const struct l0_name l0_names[] = { {"MET16", MET16}, {"BR1", BR1}, {"ELM", ELM},
			{"CARSIM", CARSIM}, {"DUMB", DUMB}, {"DUMBT", DUMBT}, {NULL,LAST}};

enum l0_nameindex set_interface;	//hw interface to use

char set_subinterface[SUBINTERFACE_MAX];		/* and sub-interface ID */

/** deprecated : **/
char *set_simfile;	//source for simulation data
extern void diag_l0_sim_setfile(char * fname);
extern unsigned int diag_l0_dumb_getopts(void);
extern void diag_l0_dumb_setopts(unsigned int);

/*
 * XXX All commands should probably have optional "init" hooks.
 */
int set_init(void)
{
	/* Reset parameters to defaults. */

	set_speed = 10400;	/* Comms speed; ECUs will probably send at 10416 bps (96us per bit) */

	global_cfg.src = 0xf1;	/* Our tester ID */
	global_cfg.addrtype = 1;	/* Use functional addressing */
	global_cfg.tgt = 0x33;	/* Dest ECU address */

	global_cfg.L1proto = DIAG_L1_ISO9141;	/* L1 protocol type */

	global_cfg.L2idx = 0;
	global_cfg.L2proto = l2proto_list[0]->diag_l2_protocol; /* cannot guarantee 9141 was compiled... DIAG_L2_PROT_ISO9141; */

	global_cfg.initmode = DIAG_L2_TYPE_FASTINIT ;

	global_cfg.units = 0;		/* English (1), or Metric (0) */

	set_vehicle = "ODBII";	/* Vehicle */
	set_ecu = "ODBII";	/* ECU name */

	set_interface_idx= DEFAULT_INTERFACE;
	set_interface = l0_names[DEFAULT_INTERFACE].code;	/* Default H/w interface to use */

	strncpy(set_subinterface,"/dev/null",SUBINTERFACE_MAX-1);
	printf( "%s: Interface set to default: %s on %s\n", progname, l0_names[set_interface_idx].longname, set_subinterface);

	if (diag_calloc(&set_simfile, strlen(DB_FILE)+1))
		return diag_iseterr(DIAG_ERR_GENERAL);
	strcpy(set_simfile, DB_FILE);			//default simfile for use with CARSIM
	diag_l0_sim_setfile(set_simfile);
	test_dl0d=NULL;

	return 0;
}

void set_close(void)
{
	if (set_simfile)
		free(set_simfile);
	return;
}



/* SET sub menu */
static int cmd_set_help(int argc, char **argv);
//static int cmd_exit(int argc, char **argv);
static int cmd_set_show(int argc, char **argv);
static int cmd_set_speed(int argc, char **argv);
static int cmd_set_testerid(int argc, char **argv);
static int cmd_set_destaddr(int argc, char **argv);
static int cmd_set_addrtype(int argc, char **argv);
static int cmd_set_l1protocol(int argc, char **argv);
static int cmd_set_l2protocol(int argc, char **argv);
static int cmd_set_initmode(int argc, char **argv);
static int cmd_set_display(int argc, char **argv);
static int cmd_set_interface(int argc, char **argv);
static int cmd_set_dumbopts(int argc, char **argb);
static int cmd_set_simfile(int argc, char **argv);

const struct cmd_tbl_entry set_cmd_table[] =
{
	{ "help", "help [command]", "Gives help for a command",
		cmd_set_help, 0, NULL},
	{ "?", "help [command]", "Gives help for a command",
		cmd_set_help, FLAG_HIDDEN, NULL},

	{ "interface", "interface NAME [dev]", "Shows/Sets the interface to use. Use set interface ? to get a list of names",
		cmd_set_interface, 0, NULL},

	{ "dumbopts", "dumbopts [opts]", "Sets dumb-interface-specific options. Use set dumbopts ? to get details.",
		cmd_set_dumbopts, 0, NULL},

	{ "simfile", "simfile [filename]", "Select simulation file to use as data input. See freediag_carsim.db for an example",
		cmd_set_simfile, FLAG_FILE_ARG, NULL},

	{ "display", "display [english/metric]", "Sets english or metric display",
		cmd_set_display, 0, NULL},

	{ "speed", "speed [speed]", "Shows/Sets the speed to connect",
		cmd_set_speed, 0, NULL},
	{ "testerid", "testerid [testerid]",
		"Shows/Sets the source ID for us to use",
		cmd_set_testerid, 0, NULL},
	{ "destaddr", "destaddr [destaddr]",
		"Shows/Sets the destination ID to connect",
		cmd_set_destaddr, 0, NULL},

	{ "addrtype", "addrtype [func/phys]", "Shows/Sets the address type to use",
		cmd_set_addrtype, 0, NULL},

	{ "l1protocol", "l1protocol [protocolname]", "Shows/Sets the hardware protocol to use. Use set l1protocol ? to get a list of protocols",
		cmd_set_l1protocol, 0, NULL},

	{ "l2protocol", "l2protocol [protocolname]", "Shows/Sets the software protocol to use. Use set l2protocol ? to get a list of protocols",
		cmd_set_l2protocol, 0, NULL},

	{ "initmode", "initmode [modename]", "Shows/Sets the initialisation mode to use. Use set initmode ? to get a list of protocols",
		cmd_set_initmode, 0, NULL},

	{ "show", "show", "Shows all set'able values",
		cmd_set_show, 0, NULL},

	{ "up", "up", "Return to previous menu level",
		cmd_up, 0, NULL},
	{ "quit","quit", "Exit program",
		cmd_exit, FLAG_HIDDEN, NULL},
	{ "exit", "exit", "Exit program",
		cmd_exit, 0, NULL},

	{ NULL, NULL, NULL, NULL, 0, NULL}
};

const char * const l1_names[] = //these MUST be in the same order as they are listed in diag_l1.h !!
{
	"ISO9141", "ISO14230",
	"J1850-VPW", "J1850-PWM", "CAN", "", "", "RAW", NULL
};

//These MUST match the DIAG_L2_TYPE_* flags in diag_l2.h  so that
// l2_initmodes[DIAG_L2_TYPE_XX] == "XX" !!
const char * const l2_initmodes[] =
{
	"5BAUD", "FAST", "CARB", NULL
};


static int
cmd_set_show(UNUSED(int argc), UNUSED(char **argv))
{
	/* Show stuff; calling the cmd_set_*() functions with argc=0 displays the current setting. */
	cmd_set_interface(0,NULL);
	if (set_interface==CARSIM)
		printf("simfile: %s\n", set_simfile);
	if (set_interface==DUMB)
		printf("dumbopts: %#02x\n", diag_l0_dumb_getopts());
	printf("speed:    Connect speed: %d\n", set_speed);
	cmd_set_display(0,NULL);
	cmd_set_testerid(0,NULL);
	cmd_set_addrtype(0,NULL);
	cmd_set_destaddr(0,NULL);
	cmd_set_l1protocol(0,NULL);
	cmd_set_l2protocol(0,NULL);
	cmd_set_initmode(0,NULL);

	/* Parse L0-specific config items */
	if (test_dl0d && test_dl0d->dl0->diag_l0_getcfg) {
		struct cfgi *cfgp= test_dl0d->dl0->diag_l0_getcfg(test_dl0d);
		for (;cfgp; cfgp = cfgp->next) {
			char *cs = diag_cfg_getstr(cfgp);
			if (cfgp->shortname == NULL || cs==NULL) continue;

			printf("L0 option: %s=%s\n",cfgp->shortname, cs);
			free(cs);
		}
	}

	return CMD_OK;
}


static int cmd_set_interface(int argc, char **argv)
{
	if (argc > 1) {
		int i, helping = 0, found = 0;
		if (strcmp(argv[1], "?") == 0) {
			helping = 1;
			printf("hardware interface: use \"set interface NAME [dev]\" .\n"
			"NAME is the interface type and [dev] is\n"
			"a complete device path such as \"/dev/ttyS0\" or \"\\\\.\\COM11\"\n"
			"Valid NAMEs are: \n");
		}
		for (i=0; l0_names[i].code != LAST; i++) {
			//loop through l0 interface names, either printing or comparing to argv[1]
			if (helping)
				printf("%s ", (l0_names[i].longname));
			else
				if (strcasecmp(argv[1], l0_names[i].longname) == 0) {
					set_interface = l0_names[i].code;
					set_interface_idx=i;
					found = 1;
					break;	//no use in continuing
				}
		}
		if (helping) {
			//"?" was entered
			int numports, i;
			char ** portlist = diag_tty_getportlist(&numports);
			printf("\nValid ports:\n");
			if (portlist) {
				for (i=0; (i < numports); i++) {
					printf("\t%s\n", portlist[i]);
				}
				strlist_free(portlist, numports);
			}
		} else if (!found) {
			printf("interface: invalid interface %s\n", argv[1]);
			printf("interface: use \"set interface ?\" to see list of names\n");
		} else {
			if (argc > 2)	//there's also a "subinterface" aka devicename
				strncpy(set_subinterface, argv[2], SUBINTERFACE_MAX-1);
			printf("interface is now %s on %s\n",
				l0_names[set_interface_idx].longname, set_subinterface);
			if (l0_names[set_interface_idx].code==DUMB) {
				printf("Note concerning generic (dumb) interfaces : there are additional\n"
					"options which can be set with \"set dumbopts\". By default\n"
					"\"K-line only\" and \"MAN_BREAK\" are set. \n");
					diag_l0_dumb_setopts(-1);	//this forces defaults.
			}
			if (l0_names[set_interface_idx].code==DUMBT)
				printf("*** Warning ! The DUMBT driver is only for electrical ***\n"
						"*** testing ! Do NOT use while connected to a vehicle! ***\n"
						"*** refer to doc/scantool-manual.html ***\n");

		}
	} else {
		printf("interface: using %s on %s\n",
			l0_names[set_interface_idx].longname, set_subinterface);
	}
	/* update current global dl0d. */
	if (test_dl0d) {
		/* XXX warn before breaking the (possibly) active L0-L2 chain */
		if (test_dl0d->dl0->diag_l0_del) test_dl0d->dl0->diag_l0_del(test_dl0d);
		test_dl0d=NULL;
	}

	if (1) {
		const struct diag_l0 *l0dev;
		int i;
		for (i=0; l0dev_list[i]; i++) {
			l0dev = l0dev_list[i];
			if (strcmp(l0_names[set_interface_idx].longname, l0dev->diag_l0_name) == 0) {
				/* Found it */
				if (l0dev->diag_l0_new) test_dl0d = l0dev->diag_l0_new();
				break;
			}
		}
	}
	return CMD_OK;
}


//Update simfile name to be used.
//Current behaviour : updates the simfile even if the interface isn't set to CARSIM.
static int cmd_set_simfile(int argc, char **argv)
{
	if (argc > 1) {
		if (strcmp(argv[1], "?") == 0) {
			printf("Simulation file: with CARSIM interface, this file contains\n"
			"message bytes to be transferred between host and ECU.\n"
			"Defaults to " DB_FILE "\n");
			return CMD_OK;
		}

		if (set_simfile)
			free(set_simfile);		//free old simfile
		if (diag_calloc(&set_simfile, strlen(argv[1])+1))
			return CMD_FAILED;
		strcpy(set_simfile, argv[1]);
		diag_l0_sim_setfile(set_simfile);
		printf("Simulation file: now using %s\n", set_simfile);

		if (set_interface!=CARSIM) {
			printf("Note: simfile only needed with CARSIM interface.\n");
		}
	} else {
		//no arguments
		printf("Simulation file: using %s\n", set_simfile);
	}
	return CMD_OK;
}

static int
cmd_set_display(int argc, char **argv)
{
	if (argc > 1)
	{
		if (strcasecmp(argv[1], "english") == 0)
			global_cfg.units = 1;
		else if (strcasecmp(argv[1], "metric") == 0)
			global_cfg.units = 0;
		else
			return CMD_USAGE;
	}
	else
		printf("display: %s units\n", global_cfg.units?"english":"metric");

	return CMD_OK;
}

static int
cmd_set_speed(int argc, char **argv)
{
	if (argc > 1)
	{
		set_speed = htoi(argv[1]);
	}
	else
		printf("speed: Connect speed: %d\n", set_speed);

	return CMD_OK;
}

static int cmd_set_dumbopts(int argc, char **argv) {
	unsigned int tmp;
	if (argc >1) {
		if ( argv[1][0]=='?' ) {
			printf("dumbopts: use \"set dumbopts [opts]\" where [opts] is the addition of the desired flags:\n"
				" 0x01 : USE_LLINE : use if the L line (driven by RTS) is required for init. Interface must support this\n"
				"\t(VAGTOOL for example).\n"
				" 0x02 : CLEAR_DTR : use if your interface needs DTR to be always clear (neg. voltage).\n"
				"\tThis is unusual. By default DTR will always be SET (pos. voltage)\n"
				" 0x04 : SET_RTS : use if your interface needs RTS to be always set (pos. voltage).\n"
				"\tThis is unusual. By default RTS will always be CLEAR (neg. voltage)\n"
				"\tThis option should not be used with USE_LLINE.\n"
				" 0x08 : MAN_BREAK : essential for USB-serial converters that don't support 5bps\n"
				"\tsuch as FTDI232*, P230* and other ICs (enabled by default).\n"
				" 0x10: LLINE_INV : Invert polarity of the L line. see\n"
				"\tdoc/dumb_interfaces.txt !! This is unusual.\n"
				" 0x20: FAST_BREAK : use alternate iso14230 fastinit code.\n"
				" 0x40: BLOCKDUPLEX : use message-based half duplex removal (if P4==0)\n\n"
				"ex.: \"dumbopts 9\" for MAN_BREAK and USE_LLINE.\n"
				"Note : these options are ignored on any non-DUMB interfaces.\n");
			return CMD_OK;
		}
		tmp=htoi(argv[1]);
		//we just set the l0 flags to whatever htoi parsed.
		diag_l0_dumb_setopts(tmp);
	}
	printf("Current dumbopts=0x%X\n", diag_l0_dumb_getopts());

	return CMD_OK;
}


static int
cmd_set_testerid(int argc, char **argv)
{
	if (argc > 1)
	{
		int tmp;
		tmp = htoi(argv[1]);
		if ( (tmp < 0) || (tmp > 0xff))
			printf("testerid: must be between 0 and 0xff\n");
		else
			global_cfg.src = (uint8_t) tmp;
	}
	else
		printf("testerid: Source ID to use: 0x%X\n", global_cfg.src);

	return CMD_OK;
}

static int
cmd_set_destaddr(int argc, char **argv)
{
	if (argc > 1) {
		int tmp;
		tmp = htoi(argv[1]);
		if ( (tmp < 0) || (tmp > 0xff))
			printf("destaddr: must be between 0 and 0xff\n");
		else
			global_cfg.tgt = (uint8_t) tmp;
	} else {
		printf("destaddr: Destination address to connect to: 0x%X\n",
			global_cfg.tgt);
	}

	return CMD_OK;
}
static int
cmd_set_addrtype(int argc, char **argv)
{
	if (argc > 1)
	{
		if (strncmp(argv[1], "func", 4) == 0)
			global_cfg.addrtype = 1;
		else if (strncmp(argv[1], "phys", 4) == 0)
			global_cfg.addrtype = 0;
		else
			return CMD_USAGE;
	}
	else
	{
		printf("addrtype: %s addressing\n",
			global_cfg.addrtype ? "functional" : "physical");
	}

	return CMD_OK;
}

static int cmd_set_l2protocol(int argc, char **argv)
{
	if (argc > 1) {
		int i, prflag = 0, found = 0;
		if (strcmp(argv[1], "?") == 0) {
			prflag = 1;
			printf("L2 protocol: valid names are ");
		}
		for (i=0; l2proto_list[i] != NULL; i++) {
			const struct diag_l2_proto *d2p=l2proto_list[i];
			if (prflag) {
					printf("%s ", d2p->shortname);
					continue;
			}
			if (strcasecmp(argv[1], d2p->shortname) == 0) {
				found = 1;
				global_cfg.L2idx = i;
				global_cfg.L2proto = d2p->diag_l2_protocol;
				break;
			}
		}
		if (prflag) {
			printf("\n");
			return CMD_OK;
		}
		if (! found) {
			printf("l2protocol: invalid protocol %s\n", argv[1]);
			printf("l2protocol: use \"set l2protocol ?\" to see list of protocols\n");
		}
	} else {
		printf("l2protocol: Layer 2 protocol to use %s\n",
			l2proto_list[global_cfg.L2idx]->shortname);
	}
	return CMD_OK;
}

static int cmd_set_l1protocol(int argc, char **argv)
{
	if (argc > 1)
	{
		int i, prflag = 0, found = 0;
		if (strcmp(argv[1], "?") == 0)
		{
			prflag = 1;
			printf("L1 protocol: valid names are ");
		}
		for (i=0; l1_names[i] != NULL; i++)
		{
			if (prflag && *l1_names[i])
				printf("%s ", l1_names[i]);
			else
				if (strcasecmp(argv[1], l1_names[i]) == 0)
				{
					global_cfg.L1proto = 1 << i;
					found = 1;
				}
		}
		if (prflag)
			printf("\n");
		else if (! found)
		{
			printf("L1protocol: invalid protocol %s\n", argv[1]);
			printf("l1protocol: use \"set l1protocol ?\" to see list of protocols\n");
		}
	}
	else
	{
		int offset;

		for (offset=0; offset < 8; offset++)
		{
			if (global_cfg.L1proto == (1 << offset))
				break;
		}
		printf("l1protocol: Layer 1 (H/W) protocol to use %s\n",
			l1_names[offset]);

	}
	return CMD_OK;
}

static int cmd_set_initmode(int argc, char **argv)
{
	if (argc > 1)
	{
		int i, prflag = 0, found = 0;
		if (strcmp(argv[1], "?") == 0)
			prflag = 1;
		for (i=0; l2_initmodes[i] != NULL; i++)
		{
			if (prflag)
				printf("%s ", l2_initmodes[i]);
			else
			{
				if (strcasecmp(argv[1], l2_initmodes[i]) == 0)
				{
					found = 1;
					global_cfg.initmode = i;
				}
			}
		}
		if (prflag)
			printf("\n");
		else if (! found)
		{
			printf("initmode: invalid mode %s\n", argv[1]);
			printf("initmode: use \"set initmode ?\" to see list of initmodes\n");
		}
	}
	else
	{
		printf("initmode: Initmode to use with above protocol is %s\n",
			l2_initmodes[global_cfg.initmode]);
	}
	return CMD_OK;
}

static int
cmd_set_help(int argc, char **argv)
{
	return help_common(argc, argv, set_cmd_table);
}
