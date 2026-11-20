/*
Pan-tilt genérico para T-SIMCAM + foco LED difuso.
Protótipo paramétrico: dimensões da placa/câmera e motores devem ser medidas antes da impressão final.
No OpenSCAD, selecione part: "assembly", "base", "motor_mount", "yoke", "camera_plate" ou "light_bracket".
Motor: "28byj" (colar para corpo cilíndrico azul) ou "bipolar" (placa com furação editável, padrão NEMA14).
Unidades: mm.
*/
$fn=64;
part="assembly";
motor_type="28byj";

// Parâmetros mecânicos editáveis
base_w=100; base_d=92; base_t=6;
turntable_w=96; turntable_d=72; turntable_t=5;
frame_w=92; frame_depth=64; frame_height=74; frame_t=5;
camera_plate_w=82; camera_plate_h=58; camera_plate_t=4;
board_slot_pitch_x=68; board_slot_pitch_y=42; slot_width=3.4; slot_length=18;
body_d=28.8; collar_clearance=0.6; collar_h=18; collar_wall=4.5;
bipolar_face=35.2; bipolar_bolt_pitch=26; bipolar_pilot_d=22;
shaft_d=5.2; shaft_flat_depth=0.7;
led_hole_d=4.2; led_slot_pitch=28;

module slot2d(len=18,w=3.4){
  hull(){ translate([-(len-w)/2,0]) circle(d=w); translate([(len-w)/2,0]) circle(d=w); }
}
module base_plate(){
  difference(){
    translate([-base_w/2,-base_d/2,0]) cube([base_w,base_d,base_t]);
    for(x=[-38,38],y=[-34,34]) translate([x,y,-1]) cylinder(d=4.2,h=base_t+2);
    for(y=[-20,20]) translate([0,y,-1]) linear_extrude(height=base_t+2) slot2d(42,4.2);
  }
}
module clamp_ring(){
  outer_d=body_d+2*collar_wall;
  difference(){
    union(){
      cylinder(d=outer_d,h=collar_h);
      // orelhas para parafuso M3 e porca; ajustar posição conforme motor real
      translate([outer_d/2-1,-4,0]) cube([8,8,collar_h]);
      translate([-outer_d/2-7,-4,0]) cube([8,8,collar_h]);
    }
    translate([0,0,-1]) cylinder(d=body_d+collar_clearance,h=collar_h+2);
    // fenda de aperto
    translate([-1.2,0,-1]) cube([2.4,outer_d/2+9,collar_h+2]);
    // furo atravessando as duas orelhas para M3; porca pode exigir ajuste local
    translate([-outer_d/2-8,0,collar_h/2]) rotate([0,90,0]) cylinder(d=3.3,h=outer_d+18);
  }
}
module bipolar_motor_plate(){
  difference(){
    translate([-bipolar_face/2-8,-bipolar_face/2-8,0]) cube([bipolar_face+16,bipolar_face+16,6]);
    translate([0,0,-1]) cylinder(d=bipolar_pilot_d+1,h=8);
    for(x=[-bipolar_bolt_pitch/2,bipolar_bolt_pitch/2],y=[-bipolar_bolt_pitch/2,bipolar_bolt_pitch/2])
      translate([x,y,-1]) cylinder(d=3.3,h=8);
    for(x=[-(bipolar_face/2+3),bipolar_face/2+3]) translate([x,0,-1]) cylinder(d=4.2,h=8);
  }
}
module motor_adapter(){
  if(motor_type=="28byj") clamp_ring();
  else bipolar_motor_plate();
}
module motor_mount(){
  // flange de montagem com quatro rasgos; o colar/placa é intercambiável
  difference(){
    translate([-32,-32,-6]) cube([64,64,6]);
    for(x=[-23,23],y=[-23,23]) translate([x,y,-7]) cylinder(d=4.2,h=8);
    translate([0,0,-7]) cylinder(d=body_d+2*collar_wall+2,h=8);
  }
  translate([0,0,0]) motor_adapter();
}
module shaft_hub(){
  difference(){
    union(){ cylinder(d=24,h=9); translate([-35,-35,0]) cube([70,70,4]); }
    // furo aliviado perno/saída; conferir perfil e comprimento real do eixo
    translate([0,0,-1]) cylinder(d=shaft_d,h=12);
    translate([shaft_d/2-shaft_flat_depth,-shaft_d,-1]) cube([3,2*shaft_d,12]);
    for(x=[-25,25],y=[-25,25]) translate([x,y,-1]) cylinder(d=3.2,h=7);
    translate([0,0,4.5]) rotate([90,0,0]) cylinder(d=3.2,h=30,center=true);
  }
}
module turntable(){
  difference(){
    translate([-turntable_w/2,-turntable_d/2,0]) cube([turntable_w,turntable_d,turntable_t]);
    for(x=[-25,25],y=[-25,25]) translate([x,y,-1]) cylinder(d=3.2,h=turntable_t+2);
    for(x=[-35,35],y=[-22,22]) translate([x,y,-1]) cylinder(d=4.2,h=turntable_t+2);
  }
}
module yoke(){
  // Bandeja giratória com laterais; montagens do eixo de tilt em rasgos laterais.
  turntable();
  translate([-frame_w/2,-frame_depth/2,turntable_t]) cube([frame_t,frame_depth,frame_height]);
  translate([frame_w/2-frame_t,-frame_depth/2,turntable_t]) cube([frame_t,frame_depth,frame_height]);
  // janela larga para passagem da câmera/cabo
  translate([-frame_w/2-1,-frame_depth/2+8,turntable_t+12]) cube([frame_w+2,frame_t+4,frame_height-20]);
  // abas/rasgos que aceitam tanto suporte de motor quanto mancal passivo
  for(x=[-frame_w/2,frame_w/2-frame_t]) translate([x,-10,turntable_t+frame_height/2]) rotate([0,90,0]) difference(){
    cylinder(d=28,h=frame_t);
    translate([0,0,-1]) cylinder(d=shaft_d+1,h=frame_t+2);
    translate([-15,-slot_width/2,-1]) cube([30,slot_width,frame_t+2]);
  }
}
module camera_plate(){
  difference(){
    translate([-camera_plate_w/2,-camera_plate_h/2,0]) cube([camera_plate_w,camera_plate_h,camera_plate_t]);
    for(x=[-board_slot_pitch_x/2,board_slot_pitch_x/2],y=[-board_slot_pitch_y/2,board_slot_pitch_y/2])
      translate([x,y,-1]) linear_extrude(height=camera_plate_t+2) slot2d(slot_length,slot_width);
    // furos para suportes/haste de tilt
    for(x=[-camera_plate_w/2+5,camera_plate_w/2-5]) translate([x,0,-1]) cylinder(d=shaft_d+0.6,h=camera_plate_t+2);
  }
}
module light_bracket(){
  // travessa frontal com rasgos para cinta, abraçadeira ou pequeno refletor difuso
  difference(){
    translate([-38,-4,0]) cube([76,8,30]);
    for(x=[-led_slot_pitch/2,led_slot_pitch/2]) translate([x,0,15]) rotate([90,0,0]) cylinder(d=led_hole_d,h=12,center=true);
    for(x=[-27,27]) translate([x,0,22]) rotate([90,0,0]) linear_extrude(height=12,center=true) slot2d(12,3.4);
  }
}
module assembly(){
  color("dimgray") base_plate();
  // bloco de motor pan e acoplamento visual; o eixo impresso é adaptador, não rolamento
  translate([0,0,base_t+2]) color("royalblue") motor_mount();
  translate([0,0,base_t+collar_h+4]) color("silver") shaft_hub();
  translate([0,0,base_t+collar_h+8]) color("slategray") yoke();
  // bandeja removível central: ajustar posição ao conjunto real
  translate([0,0,base_t+collar_h+8+turntable_t+frame_height/2]) color("orange") rotate([90,0,0]) camera_plate();
  // suporte LED frontal preso à travessa do yoke
  translate([0,-frame_depth/2-6,base_t+collar_h+8+turntable_t+frame_height-18]) color("gold") light_bracket();
}

if(part=="assembly") assembly();
else if(part=="base") base_plate();
else if(part=="motor_mount") motor_mount();
else if(part=="yoke") yoke();
else if(part=="camera_plate") camera_plate();
else if(part=="light_bracket") light_bracket();
else if(part=="shaft_hub") shaft_hub();
