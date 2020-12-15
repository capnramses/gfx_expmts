// based on https://threejsfundamentals.org/threejs/lessons/threejs-webvr.html

//import * as THREE from './resources/three/r122/build/three.module.js';
import * as THREE from './three.js/build/three.module.js';
import {VRButton} from './three.js/examples/jsm/webxr/VRButton.js';
//import {VRButton} from './resources/threejs/r122/examples/jsm/webxr/VRButton.js';

function main() {
	const canvas        = document.getElementById( "canvas" );
	const renderer      = new THREE.WebGLRenderer( { canvas } );
  renderer.xr.enabled = true;
  document.body.appendChild( VRButton.createButton( renderer ) );
	
	const fovy    = 66;
	const aspect  = 2;
	const near    = 0.01;
	const far     = 100.0;
	const camera  = new THREE.PerspectiveCamera( fovy, aspect, near, far );	
	camera.position.set( 0.0, 1.6, 0.0 );

	const scene = new THREE.Scene();
	{
		const colour    = 0xFFFFFF;
		const intensity = 1;
		const light     = new THREE.DirectionalLight( colour, intensity );
		light.position.set( -1, 2, 4 );
		scene.add( light );
	}

	const geom = new THREE.BoxGeometry( 1, 1, 1 );
	function make_inst( geom, colour, x ) {
		const material = new THREE.MeshPhongMaterial( { color:colour } );
		const cube     = new THREE.Mesh( geom, material );
		scene.add( cube );

		cube.position.x = x;
		cube.position.y = 1.6;
		cube.position.z = -2;

		return cube;
	}

	const cubes = [
		make_inst( geom, 0x44AA88,  0 ),
		make_inst( geom, 0x8844AA, -2 ),
		make_inst( geom, 0xAA8844,  2 )
	];

	function resize_renderer_to_display( renderer ) {
		const canvas = renderer.domElement;
		const width  = canvas.clientWidth;
		const height = canvas.clientHeight;
		const dirty  = canvas.width !== width || canvas.height !== height;
		if ( dirty ) { renderer.setSize( width, height, false ); }
		return dirty; 
	}

  function draw_frame( elapsed_ms ) {
	  const elapsed_s = elapsed_ms * 0.001;

	  if ( resize_renderer_to_display( renderer ) && canvas.clientHeight > 0 ) {
			const canvas  = renderer.domElement;
      camera.aspect = canvas.clientWidth / canvas.clientHeight;
      camera.updateProjectionMatrix();
    }

		cubes.forEach( ( cube, idx ) => {
			const speed     = 1 + idx * .1;
			const rot       = elapsed_s * speed;
			cube.rotation.x = rot;
			cube.rotation.y = rot;
		});

		renderer.render( scene, camera );
  }

  // let Three.js XR handle render loop ( ~2 cameras ) instead of requestAnim...
  renderer.setAnimationLoop( draw_frame );

} // endfunc main

main();
