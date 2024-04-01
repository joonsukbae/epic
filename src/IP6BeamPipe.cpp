// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) 2022 Wouter Deconinck, Whitney Armstrong, Sylvester Joosten

//==========================================================================
//
//      <detector name ="DetName" type="Beampipe" >
//      <layer id="#(int)" inner_r="#(double)" outer_z="#(double)" >
//      <slice material="string" thickness="#(double)" >
//      </layer>
//      </detector>
//==========================================================================
#include "DD4hep/DetFactoryHelper.h"
#include "DD4hep/Printout.h"
#include "TMath.h"
#include <XML/Helper.h>
#include "XML/Utilities.h"
#include "DD4hepDetectorHelper.h"

using namespace std;
using namespace dd4hep;

/** \addtogroup beamline Beamline Instrumentation
 */

/** \addtogroup IRChamber Interaction Region Vacuum Chamber.
 * \brief Type: **IRChamber**.
 * \ingroup beamline
 *
 *
 * \code
 *   <detector>
 *   </detector>
 * \endcode
 *
 */
static Ref_t create_detector(Detector& det, xml_h e, SensitiveDetector /* sens */)
{

  using namespace ROOT::Math;
  xml_det_t  x_det    = e;
  string     det_name = x_det.nameStr();
  xml_comp_t  x_dettype = x_det.child(dd4hep::xml::Strng_t("type_flags"));
  unsigned int typeFlag = x_dettype.type();
  DetElement sdet(det_name, x_det.id());
  Assembly   assembly(det_name + "_assembly");
  Material   m_Al     = det.material("Aluminum");
  Material   m_Be     = det.material("Beryllium");
  Material   m_Au     = det.material("Gold");
  Material   m_Vacuum = det.material("Vacuum");
  string     vis_name = x_det.visStr();

  xml::Component IP_pipe_c = x_det.child(_Unicode(IP_pipe));

  // IP
  double IP_beampipe_OD             = IP_pipe_c.attr<double>(_Unicode(OD));
  double IP_beampipe_wall_thickness = IP_pipe_c.attr<double>(_Unicode(wall_thickness));
  double IP_beampipe_gold_thickness = IP_pipe_c.attr<double>(_Unicode(gold_thickness));
  double IP_beampipe_ID      = IP_beampipe_OD - 2.0 * IP_beampipe_gold_thickness - 2.0 * IP_beampipe_wall_thickness;
  double IP_acts_beampipe_OD = IP_beampipe_ID - 5.0 * mm;
  double IP_acts_beampipe_ID = IP_acts_beampipe_OD - 1.0 * mm;

  double upstream_straight_length   = IP_pipe_c.attr<double>(_Unicode(upstream_straight_length));
  double downstream_straight_length = IP_pipe_c.attr<double>(_Unicode(downstream_straight_length));

  // central beampipe volume
  Tube         central_tube(0.5 * IP_acts_beampipe_ID, 0.5 * IP_acts_beampipe_OD,
                            0.5 * (upstream_straight_length + downstream_straight_length));
  Volume       central_volume("acts_central_beampipe_vol", central_tube, m_Vacuum);
  const double central_offset = -.5 * (upstream_straight_length - downstream_straight_length);
  DetElement   central_det(sdet, "acts_beampipe_central", 1);

  // Set dd4hep variant parameters for conversion to ACTS tracking geometry
  central_det.setTypeFlag(typeFlag);
  auto &params = DD4hepDetectorHelper::ensureExtension<dd4hep::rec::VariantParameters>(central_det);
  int nBinPhi = 144; // fix later. Should take this from a xml tag
  int nBinZ = 10;  // fix later. Should take this from a xml tag
  params.set<bool>("layer_material", true);
  params.set<bool>("layer_material_representing", true);
  params.set<int>("layer_material_representing_binPhi", nBinPhi);
  params.set<int>("layer_material_representing_binZ", nBinZ);

  // -----------------------------
  // IP beampipe
  //
  // setup for the central IP beampipe:
  //
  // /-------\ Be wall
  //  /-----\  Au coating
  //   /---\   Vacuum padding (5mm)
  //    /-\    Fake vacuum beampipe (1mm)
  //     -     Vacuum filled inner beampipe
  //
  Tube downstream_IP_vacuum_fill(0.0, IP_acts_beampipe_ID / 2.0, downstream_straight_length / 2.0);
  Tube downstream_IP_acts_beampipe(IP_acts_beampipe_ID / 2.0, IP_acts_beampipe_OD / 2.0,
                                   downstream_straight_length / 2.0);
  Tube downstream_IP_vacuum_padding(IP_acts_beampipe_OD / 2.0, IP_beampipe_ID / 2.0, downstream_straight_length / 2.0);
  Tube downstream_IP_gold(IP_beampipe_ID / 2.0, IP_beampipe_ID / 2.0 + IP_beampipe_gold_thickness,
                          downstream_straight_length / 2.0);
  Tube downstream_IP_tube(IP_beampipe_ID / 2.0 + IP_beampipe_gold_thickness, IP_beampipe_OD / 2.0,
                          downstream_straight_length / 2.0);

  Tube upstream_IP_vacuum_fill(0.0, IP_acts_beampipe_ID / 2.0, upstream_straight_length / 2.0);
  Tube upstream_IP_acts_beampipe(IP_acts_beampipe_ID / 2.0, IP_acts_beampipe_OD / 2.0, upstream_straight_length / 2.0);
  Tube upstream_IP_vacuum_padding(IP_acts_beampipe_OD / 2.0, IP_beampipe_ID / 2.0, upstream_straight_length / 2.0);
  Tube upstream_IP_gold(IP_beampipe_ID / 2.0, IP_beampipe_ID / 2.0 + IP_beampipe_gold_thickness,
                        upstream_straight_length / 2.0);
  Tube upstream_IP_tube(IP_beampipe_ID / 2.0 + IP_beampipe_gold_thickness, IP_beampipe_OD / 2.0,
                        upstream_straight_length / 2.0);

  Volume v_downstream_IP_vacuum_fill("v_downstream_IP_vacuum_fill", downstream_IP_vacuum_fill, m_Vacuum);
  Volume v_downstream_IP_acts_beampipe("v_downstream_IP_acts_beampipe", downstream_IP_acts_beampipe, m_Vacuum);
  Volume v_downstream_IP_vacuum_padding("v_downstream_IP_vacuum_padding", downstream_IP_vacuum_padding, m_Vacuum);
  Volume v_downstream_IP_gold("v_downstream_IP_gold", downstream_IP_gold, m_Au);
  Volume v_downstream_IP_tube("v_downstream_IP_tube", downstream_IP_tube, m_Be);
  Volume v_upstream_IP_vacuum_fill("v_upstream_IP_vacuum_fill", upstream_IP_vacuum_fill, m_Vacuum);
  Volume v_upstream_IP_acts_beampipe("v_upstream_IP_acts_beampipe", upstream_IP_acts_beampipe, m_Vacuum);
  Volume v_upstream_IP_vacuum_padding("v_upstream_IP_vacuum_padding", upstream_IP_vacuum_padding, m_Vacuum);
  Volume v_upstream_IP_gold("v_upstream_IP_gold", upstream_IP_gold, m_Au);
  Volume v_upstream_IP_tube("v_upstream_IP_tube", upstream_IP_tube, m_Be);

  sdet.setAttributes(det, v_upstream_IP_gold, x_det.regionStr(), x_det.limitsStr(), vis_name);
  sdet.setAttributes(det, v_upstream_IP_tube, x_det.regionStr(), x_det.limitsStr(), vis_name);
  sdet.setAttributes(det, v_downstream_IP_gold, x_det.regionStr(), x_det.limitsStr(), vis_name);
  sdet.setAttributes(det, v_downstream_IP_tube, x_det.regionStr(), x_det.limitsStr(), vis_name);

  assembly.placeVolume(v_upstream_IP_vacuum_fill, Position(0, 0, -upstream_straight_length / 2.0));
  central_volume.placeVolume(v_upstream_IP_acts_beampipe,
                             Position(0, 0, -upstream_straight_length / 2.0 - central_offset));
  assembly.placeVolume(v_upstream_IP_vacuum_padding, Position(0, 0, -upstream_straight_length / 2.0));
  assembly.placeVolume(v_upstream_IP_gold, Position(0, 0, -upstream_straight_length / 2.0));
  assembly.placeVolume(v_upstream_IP_tube, Position(0, 0, -upstream_straight_length / 2.0));

  assembly.placeVolume(v_downstream_IP_vacuum_fill, Position(0, 0, downstream_straight_length / 2.0));
  central_volume.placeVolume(v_downstream_IP_acts_beampipe,
                             Position(0, 0, downstream_straight_length / 2.0 - central_offset));
  assembly.placeVolume(v_downstream_IP_vacuum_padding, Position(0, 0, downstream_straight_length / 2.0));
  assembly.placeVolume(v_downstream_IP_gold, Position(0, 0, downstream_straight_length / 2.0));
  assembly.placeVolume(v_downstream_IP_tube, Position(0, 0, downstream_straight_length / 2.0));

  auto central_pv = assembly.placeVolume(central_volume, Position(0, 0, +central_offset));
  central_det.setPlacement(central_pv);

  //---------------------------------------------------------------------------------
  // Helper function to create polycone pairs (matter and vacuum have separate sizes)
  //
  //  ......../    /..
  //  ......./    /...
  //  ______/    /....
  //            /.....
  //  _________/......
  //  ................

  auto zplane_to_polycones = [](xml::Component& x_pipe) {
    // vacuum
    std::vector<double> rmax_vac, rmin_vac, z_vac;
    xml::Component vacuum = x_pipe.child(_Unicode(vacuum));
    for (xml_coll_t x_zplane_i(vacuum, _Unicode(zplane)); x_zplane_i; ++x_zplane_i) {
      xml_comp_t x_zplane  = x_zplane_i;
      rmin_vac.push_back(0);
      rmax_vac.push_back(x_zplane.attr<double>(_Unicode(OD)) / 2.0);
      z_vac.push_back(x_zplane.attr<double>(_Unicode(z)));
    }
    // matter
    std::vector<double> rmax_mat, rmin_mat, z_mat;
    xml::Component matter = x_pipe.child(_Unicode(matter));
    for (xml_coll_t x_zplane_i(matter, _Unicode(zplane)); x_zplane_i; ++x_zplane_i) {
      xml_comp_t x_zplane  = x_zplane_i;
      rmin_mat.push_back(0);
      rmax_mat.push_back(x_zplane.attr<double>(_Unicode(OD)) / 2.0);
      z_mat.push_back(x_zplane.attr<double>(_Unicode(z)));
    }
    return std::make_pair<Polycone, Polycone>({0, 2.0 * M_PI, rmin_mat, rmax_mat, z_mat},
      {0, 2.0 * M_PI, rmin_vac, rmax_vac, z_vac});
  };
  //---------------------------------------------------------------------------------
  // Create volumes
  auto create_volumes = [&](const std::string& name, xml::Component& x_pipe1, xml::Component& x_pipe2,
                            xml_coll_t& x_additional_subtraction_i, bool subtract_vacuum_from_matter = true,
                            bool subtract_matter_from_vacuum = false) {
    auto pipe1_polycones = zplane_to_polycones(x_pipe1);
    auto pipe2_polycones = zplane_to_polycones(x_pipe2);

    auto crossing_angle    = getAttrOrDefault(x_pipe2, _Unicode(crossing_angle), 0.0);
    auto axis_intersection = getAttrOrDefault(x_pipe2, _Unicode(axis_intersection), 0.0);

    auto tf = Transform3D(Position(0, 0, axis_intersection)) * Transform3D(RotationY(crossing_angle)) *
              Transform3D(Position(0, 0, -axis_intersection));

    // union of all matter and vacuum
    UnionSolid matter_union(pipe1_polycones.first, pipe2_polycones.first, tf);
    UnionSolid vacuum_union(pipe1_polycones.second, pipe2_polycones.second, tf);

    // subtract vacuum from matter
    BooleanSolid matter;
    if (subtract_vacuum_from_matter) {
      matter = SubtractionSolid(matter_union, vacuum_union);
    } else {
      matter = matter_union;
    }
    // subtract matter from vacuum
    BooleanSolid vacuum;
    if (subtract_matter_from_vacuum) {
      vacuum = SubtractionSolid(vacuum_union, matter_union);
    } else {
      vacuum = vacuum_union;
    }

    // subtract additional vacuum from matter
    for (; x_additional_subtraction_i; ++x_additional_subtraction_i) {
      xml_comp_t x_additional_subtraction  = x_additional_subtraction_i;
      auto       additional_polycones      = zplane_to_polycones(x_additional_subtraction);
      auto       additional_crossing_angle = getAttrOrDefault(x_additional_subtraction, _Unicode(crossing_angle), 0.0);
      auto additional_axis_intersection = getAttrOrDefault(x_additional_subtraction, _Unicode(axis_intersection), 0.0);
      auto additional_tf                = Transform3D(Position(0, 0, additional_axis_intersection)) *
                           Transform3D(RotationY(additional_crossing_angle)) *
                           Transform3D(Position(0, 0, -additional_axis_intersection));
      matter = SubtractionSolid(matter, additional_polycones.second, additional_tf);
    }

    return std::make_pair<Volume, Volume>({"v_" + name + "_matter", matter, m_Al},
                                          {"v_" + name + "_vacuum", vacuum, m_Vacuum});
  };
  //---------------------------------------------------------------------------------
  // Create downstream volumes
  auto create_volumes_2 = [&](const std::string& name, xml::Component& x_pipe1, xml::Component& x_pipe2,
                              xml::Component& x_additional_subtraction, bool subtract_vacuum_from_matter = true,
                              bool subtract_matter_from_vacuum = false) {
    auto pipe1_polycones = zplane_to_polycones(x_pipe1);
    auto pipe2_polycones = zplane_to_polycones(x_pipe2);

    auto crossing_angle      = getAttrOrDefault(x_pipe2, _Unicode(crossing_angle), 0.0);
    auto axis_intersection   = getAttrOrDefault(x_pipe2, _Unicode(axis_intersection), 0.0);
//    auto thickness_pipe1   = getAttrOrDefault(x_pipe1, _Unicode(thickness), 0.0);
    auto thickness_pipe2     = getAttrOrDefault(x_pipe2, _Unicode(thickness), 0.0);
    auto horizontal_offset   = getAttrOrDefault(x_pipe2, _Unicode(horizontal_offset), 0.0);
    auto cone_z_end          = getAttrOrDefault(x_pipe2, _Unicode(cone_z_end), 0.0);
    auto cone_z_start        = getAttrOrDefault(x_pipe2, _Unicode(cone_z_start), 0.0);
    auto extension_r         = getAttrOrDefault(x_pipe2, _Unicode(extension_r), 0.0);
    auto extension_z         = getAttrOrDefault(x_pipe2, _Unicode(extension_z), 0.0);
    auto extension_thickness = getAttrOrDefault(x_pipe2, _Unicode(extension_thickness), 0.0);

    // transformation matrix: shift -> rotate -> shift
    auto tf = Transform3D(Position(0, 0, axis_intersection)) *
              Transform3D(RotationY( std::abs(crossing_angle))) *
              Transform3D(Position(0, 0, -axis_intersection));

    Solid pipe2_polycones_mat_cut, pipe2_polycones_vac_cut;
    // virtual box to subtract from solids
    const double box_cut_sizex = 10000.0 * mm;
    const double box_cut_sizey = 10000.0 * mm;
    const double box_cut_sizez = 10000.0 * mm;
    Box box_cut(box_cut_sizex,box_cut_sizey,box_cut_sizez); // virtual box to subtract from solids

    // horizontal offset of the outgoing h-beam pipe w.r.t. the incoming e-beam pipe
    const double p_mat_1[] = {horizontal_offset, 0, 0}; // matter
    const double p_vac_1[] = {horizontal_offset - thickness_pipe2, 0, 0}; // vacuum

    pipe2_polycones_mat_cut = SubtractionSolid(pipe2_polycones.first, box_cut,
                                tf * Transform3D(Position(p_mat_1[0] + box_cut_sizex,
                                                          p_mat_1[1],
                                                          p_mat_1[2])));
    pipe2_polycones_vac_cut = SubtractionSolid(pipe2_polycones.second, box_cut,
                                tf * Transform3D(Position(p_vac_1[0] + box_cut_sizex,
                                                          p_vac_1[1],
                                                          p_vac_1[2])));
    // cut on the opposite side from the IP
    const double p_mat_2[] = {0, 0, cone_z_end + thickness_pipe2}; // matter
    const double p_vac_2[] = {0, 0, cone_z_end}; // vacuum

    pipe2_polycones_mat_cut = SubtractionSolid(pipe2_polycones_mat_cut, box_cut,
                                tf * Transform3D(Position(p_mat_2[0],
                                                          p_mat_2[1],
                                                          p_mat_2[2] + box_cut_sizez)));
    pipe2_polycones_vac_cut = SubtractionSolid(pipe2_polycones_vac_cut, box_cut,
                                tf * Transform3D(Position(p_vac_2[0],
                                                          p_vac_2[1],
                                                          p_vac_2[2] + box_cut_sizez)));

    // cut on the IP side
    const double p_mat_3[] = {0, 0, cone_z_start - thickness_pipe2}; // matter
    const double p_vac_3[] = {0, 0, cone_z_start}; // vacuum

    pipe2_polycones_mat_cut = SubtractionSolid(pipe2_polycones_mat_cut, box_cut,
                                tf * Transform3D(Position(p_mat_3[0],
                                                          p_mat_3[1],
                                                          p_mat_3[2] - box_cut_sizez)));
    pipe2_polycones_vac_cut = SubtractionSolid(pipe2_polycones_vac_cut, box_cut,
                                tf * Transform3D(Position(p_vac_3[0],
                                                          p_vac_3[1],
                                                          p_vac_3[2] - box_cut_sizez)));
    // add an extension to the h-beam pipe
    Tube tb_vac(0 * mm, extension_r, extension_z);
    Tube tb_mat(0 * mm, extension_r + extension_thickness, extension_z);

    pipe2_polycones_vac_cut = UnionSolid(pipe2_polycones_vac_cut, tb_vac,
                                tf * Transform3D(RotationY(crossing_angle)) *
                                     Transform3D(Position(0, 0, cone_z_end)));

    pipe2_polycones_mat_cut = UnionSolid(pipe2_polycones_mat_cut, tb_mat,
                                tf * Transform3D(RotationY(crossing_angle)) *
                                     Transform3D(Position(0, 0, cone_z_end)));

     // subtract vacuum from matter
    if (subtract_vacuum_from_matter) {
      pipe2_polycones_mat_cut = SubtractionSolid(pipe2_polycones_mat_cut,pipe2_polycones_vac_cut);
    }
    // unite matter h-beam and e-beam pipes
    pipe2_polycones_mat_cut = UnionSolid(pipe2_polycones_mat_cut,pipe1_polycones.first,tf);
    // subtract e-vacuum from matter
    Solid matter;
    if (subtract_vacuum_from_matter) {
      matter = SubtractionSolid(pipe2_polycones_mat_cut,pipe1_polycones.second,tf);
    }
    else {
      matter = pipe2_polycones_mat_cut;
    }
    // unite h- and e-vacuum
    Solid vacuum = UnionSolid(pipe2_polycones_vac_cut,pipe1_polycones.second,tf);

    // subtract matter from vacuum
    if (subtract_matter_from_vacuum) {
      vacuum = SubtractionSolid(vacuum,matter);
    }

    // subtract additional vacuum from matter and add it to the vacuum
    auto additional_polycones = zplane_to_polycones(x_additional_subtraction);
    auto additional_crossing_angle = getAttrOrDefault(x_additional_subtraction, _Unicode(crossing_angle), 0.0);
    auto additional_tf = Transform3D(RotationY(additional_crossing_angle));

    // final matter and vacuum
    matter = SubtractionSolid(matter,additional_polycones.second,tf * additional_tf);
    vacuum = UnionSolid(vacuum,additional_polycones.second,tf * additional_tf);

    return std::make_pair<Volume, Volume>({"v_" + name + "_matter", matter, m_Al},
                                          {"v_" + name + "_vacuum", vacuum, m_Vacuum});
  };
  //---------------------------------------------------------------------------------

  // -----------------------------
  // Upstream:
  // - incoming hadron tube: straight section, tapered section, straight section
  // - outgoing electron tube: tapered section, straight section

  xml::Component upstream_c        = x_det.child(_Unicode(upstream));
  xml::Component incoming_hadron_c = upstream_c.child(_Unicode(incoming_hadron));
  xml::Component outgoing_lepton_c = upstream_c.child(_Unicode(outgoing_lepton));
  xml_coll_t     additional_subtractions_upstream(upstream_c, _Unicode(additional_subtraction));
  bool           subtract_vacuum_upstream = getAttrOrDefault<bool>(upstream_c, _Unicode(subtract_vacuum), true);
  bool           subtract_matter_upstream = getAttrOrDefault<bool>(upstream_c, _Unicode(subtract_matter), true);
  auto           volumes_upstream =
      create_volumes("upstream", outgoing_lepton_c, incoming_hadron_c, additional_subtractions_upstream,
                     subtract_vacuum_upstream, subtract_matter_upstream);

  auto tf_upstream = Transform3D(RotationZYX(0, 0, 0));
  if (getAttrOrDefault<bool>(upstream_c, _Unicode(reflect), true)) {
    tf_upstream = Transform3D(RotationZYX(0, M_PI, 0));
  }
  assembly.placeVolume(volumes_upstream.first, tf_upstream);
  if (getAttrOrDefault<bool>(upstream_c, _Unicode(place_vacuum), true)) {
    assembly.placeVolume(volumes_upstream.second, tf_upstream);
  }

  // -----------------------------
  // downstream:
  // - incoming electron tube: tube with tube cut out
  // - outgoing hadron tube: cone centered at scattering angle
  // (incoming electron tube internal touching to outgoing hadron tube)

  xml::Component downstream_c      = x_det.child(_Unicode(downstream));
  xml::Component incoming_lepton_c = downstream_c.child(_Unicode(incoming_lepton));
  xml::Component outgoing_hadron_c = downstream_c.child(_Unicode(outgoing_hadron));
  xml::Component additional_subtractions_downstream_c = downstream_c.child(_Unicode(additional_subtraction));
  bool           subtract_vacuum_downstream = getAttrOrDefault<bool>(downstream_c, _Unicode(subtract_vacuum), true);
  bool           subtract_matter_downstream = getAttrOrDefault<bool>(downstream_c, _Unicode(subtract_matter), true);

  auto volumes_downstream = create_volumes_2("downstream", incoming_lepton_c, outgoing_hadron_c,
    additional_subtractions_downstream_c, subtract_vacuum_downstream, subtract_matter_downstream);

  // transform
  auto tf_downstream = Transform3D(Position(0, 0, +getAttrOrDefault(outgoing_hadron_c, _Unicode(axis_intersection), 0.0))) *
                       Transform3D(RotationY(getAttrOrDefault(outgoing_hadron_c, _Unicode(crossing_angle), 0.0))) *
                       Transform3D(Position(0, 0, -getAttrOrDefault(outgoing_hadron_c, _Unicode(axis_intersection), 0.0)));

  // reflect
  if(getAttrOrDefault<bool>(downstream_c, _Unicode(reflect), true))
    tf_downstream = Transform3D(RotationZYX(0, M_PI, 0));

  // add matter
  assembly.placeVolume(volumes_downstream.first, tf_downstream);

  // add vacuum
  if(getAttrOrDefault<bool>(downstream_c, _Unicode(place_vacuum), true))
    assembly.placeVolume(volumes_downstream.second, tf_downstream);

  // -----------------------------
  // final placement
  auto pv_assembly = det.pickMotherVolume(sdet).placeVolume(assembly);
  pv_assembly.addPhysVolID("system", sdet.id());
  sdet.setPlacement(pv_assembly);
  assembly->GetShape()->ComputeBBox();

  return sdet;
}

DECLARE_DETELEMENT(IP6BeamPipe, create_detector)
