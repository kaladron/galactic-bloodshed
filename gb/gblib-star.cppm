// SPDX-License-Identifier: Apache-2.0

export module gblib:star;

import :types;
import :tweakables;

export struct star_struct {
  unsigned short ships;            /* 1st ship in orbit */
  char name[NAMESIZE];             /* name of star */
  governor_t governor[MAXPLAYERS]; /* which subordinate maintains the system */
  ap_t AP[MAXPLAYERS];             /* action pts alotted */
  uint64_t explored;               /* who's been here 64 bits*/
  uint64_t inhabited;              /* who lives here now 64 bits*/
  double xpos, ypos;

  unsigned char numplanets;            /* # of planets in star system */
  char pnames[MAXPLANETS][NAMESIZE];   /* names of planets */
  unsigned long planetpos[MAXPLANETS]; /* file posns of planets */

  unsigned char stability;   /* how close to nova it is */
  unsigned char nova_stage;  /* stage of nova */
  unsigned char temperature; /* factor which expresses how hot the star is*/
  double gravity;            /* attraction of star in "Standards". */

  starnum_t star_id;
  long dummy[1]; /* dummy bits for development */
};

export class Star {
 public:
  [[nodiscard]] std::string get_name() const { return star_struct.name; }
  void set_name(std::string_view name) {
    std::strncpy(star_struct.name, name.data(), NAMESIZE - 1);
  }

  [[nodiscard]] std::string get_planet_name(planetnum_t pnum) const {
    return star_struct.pnames[pnum];
  }
  void set_planet_name(planetnum_t pnum, std::string_view name) {
    std::strncpy(star_struct.pnames[pnum], name.data(), NAMESIZE - 1);
  }
  [[nodiscard]] bool planet_name_isset(planetnum_t pnum) const {
    return star_struct.pnames[pnum] != nullptr;
  };

  // This is used both as a boolean and a setter.
  uint64_t& explored() { return star_struct.explored; }
  [[nodiscard]] uint64_t explored() const { return star_struct.explored; }

  uint64_t& inhabited() { return star_struct.inhabited; }
  [[nodiscard]] uint64_t inhabited() const { return star_struct.inhabited; }

  [[nodiscard]] int numplanets() const { return star_struct.numplanets; }

  double& xpos() { return star_struct.xpos; }
  [[nodiscard]] double xpos() const { return star_struct.xpos; }

  double& ypos() { return star_struct.ypos; }
  [[nodiscard]] double ypos() const { return star_struct.ypos; }

  ap_t& AP(player_t playernum) { return star_struct.AP[playernum]; }

  // which subordinate maintains the system
  governor_t& governor(player_t playernum) {
    return star_struct.governor[playernum];
  }
  // which subordinate maintains the system
  [[nodiscard]] governor_t governor(player_t playernum) const {
    return star_struct.governor[playernum];
  }

  /* 1st ship in orbit */
  unsigned short& ships() { return star_struct.ships; }

  // how close to nova it is
  unsigned char& stability() { return star_struct.stability; }

  // stage of nova
  unsigned char& nova_stage() { return star_struct.nova_stage; }

  // factor which expresses how hot the star is
  unsigned char& temperature() { return star_struct.temperature; }

  // attraction of star in "Standards".
  double& gravity() { return star_struct.gravity; }

  int control(player_t, governor_t);

  [[nodiscard]] star_struct get_struct() const { return star_struct; }

  Star(star_struct in) : star_struct(in) {}

 private:
  star_struct star_struct{};
};

/* this data will all be read at once */
export struct stardata {
  unsigned short numstars; /* # of stars */
  unsigned short ships;    /* 1st ship in orbit */
  ap_t AP[MAXPLAYERS];     /* Action pts for each player */
  unsigned short VN_hitlist[MAXPLAYERS];
  /* # of ships destroyed by each player */
  char VN_index1[MAXPLAYERS]; /* negative value is used */
  char VN_index2[MAXPLAYERS]; /* VN's record of destroyed ships
                                        systems where they bought it */
  unsigned long dummy[2];
};
