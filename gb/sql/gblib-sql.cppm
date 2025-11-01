// SPDX-License-Identifier: Apache-2.0

export module gblib:sql;

import :gameobj;
import :ships;
import :star;

export class Sql : public Db {
 public:
  Sql();
  explicit Sql(const std::string& db_path);
  virtual ~Sql() override;
  virtual int Numcommods() override;
  virtual player_t Numraces() override;
  virtual void putcommod(const Commod&, int);
  virtual void putship(Ship*);
  virtual void putstar(const Star&, starnum_t);
  virtual void putrace(const Race&);
  virtual void putsdata(stardata*);
  virtual void getsdata(stardata*);
  virtual Race getrace(player_t);
  virtual Star getstar(starnum_t);
  virtual std::optional<Ship> getship(const shipnum_t shipnum);
  virtual std::optional<Ship> getship(Ship**, const shipnum_t);
  virtual Commod getcommod(commodnum_t);
  virtual Planet getplanet(const starnum_t, const planetnum_t);
  virtual void putplanet(const Planet&, const Star&, const planetnum_t);
};
