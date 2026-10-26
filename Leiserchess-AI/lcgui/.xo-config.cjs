module.exports = {
	extends: ['xo-react', 'plugin:react/jsx-runtime'],
	prettier: true,
	rules: {
		'import/extensions': ['off'],
		'n/file-extension-in-import': ['off'],
		'no-constant-condition': ['error', { checkLoops: false }],
	},
	envs: ['es2022', 'node'],
};
